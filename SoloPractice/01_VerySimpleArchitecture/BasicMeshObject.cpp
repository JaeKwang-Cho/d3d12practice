// BasicMeshObject.cpp from "megayuchi"

#include "pch.h"
#include "BasicMeshObject.h"
#include "typedef.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "D3DUtil.h"
#include "ConstantBufferPool.h"
#include "DescriptorPool.h"

#pragma comment(lib, "D3DCompiler.lib")

Microsoft::WRL::ComPtr<ID3D12RootSignature> BasicMeshObject::m_pRootSignature = nullptr;
Microsoft::WRL::ComPtr<ID3D12PipelineState> BasicMeshObject::m_pPipelineState = nullptr;
DWORD BasicMeshObject::m_dwInitRefCount = 0;

bool BasicMeshObject::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;

	bool bResult = InitCommonResources();
	return bResult;
}

void BasicMeshObject::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld)
{
	// D3D12는 완전 비동기 API이고, GPU와 CPU의 타임라인이 동기화 되어있지 않다.
	// 그래서 draw를 할 때, shader에 넘어가는 resource들이 구분되어서 안전하게 있어야 한다.
	// 여기서는 descriptor table과 constant buffer를 말한다.
	// 그래서 이 함수가 호출 될 때마다, CBV pool과 Descriptor pool에서 하나씩 빼와야 한다.

	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	// Renderer가 관리하는 Pool
	ConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool();
	DescriptorPool* pDescriptorPool = m_pRenderer->INL_DescriptorPool();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pPoolDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	// 여기서 몇개의 Texture와 CB가 넘어갈지 계산한 다음
	DWORD dwRequiredDescriptorCount = DESCRIPTOR_COUNT_PER_OBJ + (m_dwTriGroupCount * DESCRIPTOR_COUNT_PER_TRI_GROUP);
	// 그 개수만큼 pool에서 할당 받는다.
	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, dwRequiredDescriptorCount)) {
		__debugbreak();
	}

	// 내부적으로는 커다란 Resource를 나눠쓰는 것이고, Constant Buffer View가 pointer 역할을 하는 것이다.
	CB_CONTAINER* pCB = pConstantBufferPool->Alloc();
	if (!pCB) {
		__debugbreak();
	}

	CONSTANT_BUFFER_DEFAULT* pConstantBufferDefault = reinterpret_cast<CONSTANT_BUFFER_DEFAULT*>(pCB->pSystemMemAddr);
	// CB 값을 업데이트 하고
	XMMATRIX viewMat;
	XMMATRIX projMat;
	m_pRenderer->GetViewProjMatrix(&viewMat, &projMat);

	pConstantBufferDefault->matView = XMMatrixTranspose(viewMat);
	pConstantBufferDefault->matProj = XMMatrixTranspose(projMat);
	pConstantBufferDefault->matWorld = XMMatrixTranspose(*_pMatWorld);

	XMMATRIX wvpMat = (*_pMatWorld) * viewMat * projMat;

	pConstantBufferDefault->matWVP = XMMatrixTranspose(wvpMat);

	// 초기화 할 때 정한 Texture와 업데이트한 CB를 넘길, Descriptor Table을 구성한다.

	// Object 마다 넘어가는 CB는 CopyDescriptorSimple을 이용해서, 현재 Constant Buffer View를 현재 할당된 Descriptor Heap 위치에 있는 Descriptor에 복사한다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(cpuDescriptorTable, static_cast<INT>(BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ::CBV), srvDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, dest, pCB->cbvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // CPU 코드에서는 CPU Handle에 write만 가능하다.

	// pool에서 allocation 받았기 때문에 선형인 Heap위에 있기에 
	dest.Offset(1, srvDescriptorSize); // 이렇게 같은 핸들을 offset을 이용해 다음 자리를 구하면 된다.

	// 그리고, Triangle 마다 넘어가는 SRV(Texture)를 복사한다.
	for (DWORD i = 0; i < m_dwTriGroupCount; i++) {
		INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList + i;
		TEXTURE_HANDLE* pTexHandle = pTriGroup->pTexHandle;
		if (pTexHandle) 
		{
			pD3DDevice->CopyDescriptorsSimple(1, dest, pTexHandle->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else 
		{
			__debugbreak();
		}
		dest.Offset(1, srvDescriptorSize);
	}


	// 루트 시그니처를 설정하고
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// pool에 있는 Heap으로 설정한다.
	_pCommandList->SetDescriptorHeaps(1, pPoolDescriptorHeap.GetAddressOf());

	// 일단 테이블 번호에 맞춰서 0번으로 CBV를 넘겨준다. 지금은 1개만 넘겨준다.
	_pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

	// 그리고 Texture SRV를 바꾸면서 bind 해주고, 삼각형을 그린다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTriGroup(gpuDescriptorTable, DESCRIPTOR_COUNT_PER_OBJ, srvDescriptorSize);
	for (DWORD i = 0; i < m_dwTriGroupCount; i++) 
	{
		// 테이블 번호 1번에 넣어주면서 삼각형을 그린다.
		_pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTriGroup);

		INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList + i;
		_pCommandList->IASetIndexBuffer(&(pTriGroup->indexBufferView));
		_pCommandList->DrawIndexedInstanced(pTriGroup->dwTriCount * 3, 1, 0, 0, 0);

		// 다음 텍스쳐를 bind 해준다.
		gpuDescriptorTableForTriGroup.Offset(1, srvDescriptorSize);
	}
}

bool BasicMeshObject::BeginCreateMesh(const BasicVertex* _pVertexList, DWORD _dwVertexNum, DWORD _dwTriGroupCount)
{
	bool bResult = false;
	
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	if (_dwTriGroupCount > MAX_TRI_GROUP_COUNT_PER_OBJ) {
		__debugbreak();
	}
	// 일단 Vertex Buffer 먼저 생성한다.
	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), _dwVertexNum, &m_VertexBufferView, &m_pVertexBuffer, (void*)_pVertexList))) {
		__debugbreak();
		goto RETURN;
	}

	m_dwMaxTriGroupCount = _dwTriGroupCount;
	m_pTriGroupList = new INDEXED_TRI_GROUP[m_dwMaxTriGroupCount];
	memset(m_pTriGroupList, 0, sizeof(INDEXED_TRI_GROUP) * m_dwMaxTriGroupCount);

	bResult = true;

RETURN:
	return bResult;
}

bool BasicMeshObject::InsertIndexedTriList(const uint16_t* _pIndexList, DWORD _dwTriCount, const WCHAR* _wchTexFileName)
{
	bool bResult = false;
	
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();
	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();

	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

	if (m_dwTriGroupCount >= m_dwMaxTriGroupCount) {
		__debugbreak();
		goto RETURN;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer(_dwTriCount * 3, &indexBufferView, &pIndexBuffer, (void*)_pIndexList))) {
		__debugbreak();
		goto RETURN;
	}
	// 인덱싱한 삼각형 그룹들을 만든다.
	{
		INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList + m_dwTriGroupCount;
		pTriGroup->pIndexBuffer = pIndexBuffer;
		pTriGroup->indexBufferView = indexBufferView;
		pTriGroup->dwTriCount = _dwTriCount;
		pTriGroup->pTexHandle = (TEXTURE_HANDLE*)m_pRenderer->CreateTextureFromFile(_wchTexFileName);
		m_dwTriGroupCount++;

		bResult = true;
	}
RETURN:
	return false;
}

void BasicMeshObject::EndCreateMesh()
{
}

bool BasicMeshObject::InitCommonResources()
{
	// root signature와 PSO를 싱글톤으로 사용한다.
	if (m_dwInitRefCount > 0) {
		goto EXIST;
	}

	InitRootSignature();
	InitPipelineState();

EXIST:
	m_dwInitRefCount++;
	return true;
}

void BasicMeshObject::CleanupSharedResources()
{
	if (!m_dwInitRefCount) {
		return;
	}

	DWORD refCount = --m_dwInitRefCount;
	// 아직 참조하는 Object가 있다면 삭제하지 않기
	if (!refCount) {
		// 얘네는 data section에 있는 애들이라 직접 해제를 해줘야 한다.
		m_pRootSignature = nullptr;
		m_pPipelineState = nullptr;
	}
}

bool BasicMeshObject::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();


	// 중간에 Serialize를 도와줄 blob을 만들고
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

	// CB와 Texture가 넘어가는걸 표현해줄 root signature를 만든다.
	// GPU는 똑같이 받아들이지만, CPU에서는 의미적으로 많이 다르고,
	// 데이터 넘기는 것도, 요청하는 것도 다르기 때문에 Entry를 분리해서 해준다.
	CD3DX12_DESCRIPTOR_RANGE rangePerObj[1] = {};
	rangePerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 :  Object 마다 넘기는 Constant Buffer View

	CD3DX12_DESCRIPTOR_RANGE rangePerTri[1] = {};
	rangePerTri[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 : 삼각형 마다 넘기는 texture
	
	// table 0번과 1번에 저장한다.
	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangePerObj), rangePerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangePerTri), rangePerTri, D3D12_SHADER_VISIBILITY_ALL);


	// Texture Sample을 할때 사용하는 Sampler를
	// static sampler로 Root Signature와 함께 넘겨준다.
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0); // s0 : sampler
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// 이제 Table Entry와 Sampler를 넘겨주면서 Root Signature를 만든다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	// Serialize 한 다음
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignatureBlob.GetAddressOf(), pErrorBlob.GetAddressOf()))) {
		__debugbreak();
	}
	// Root signature를 만든다.
	if (FAILED(pD3DDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf())))) {
		__debugbreak();
	}

	return true;
}

bool BasicMeshObject::InitPipelineState()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

#if defined(_DEBUG)
	// 쉐이더를 컴파일 할때, 최적화를 안하고 디버깅하기 편하도록 하는 것이다.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlas = 0;
#endif
	HRESULT hr;

	// vertex shader를 컴파일하고
	hr = D3DCompileFromFile(L".\\Shaders\\Simple.hlsl", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, pVertexShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		// 메모 : 왜 때문인지 D3DCompiler_47.dll 로드 오류가 뜬다.
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}
	// pixel shader도 컴파일 한다.
	hr = D3DCompileFromFile(L".\\Shaders\\Simple.hlsl", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, pPixelShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		// 메모 : 왜 때문인지 D3DCompiler_47.dll 로드 오류가 뜬다.
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}

	// input layout을 정의하고
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) + sizeof(XMFLOAT4),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Pipeline State Object (PSO)를 만든다.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = true; // 이제 depth 버퍼를 사용한다.
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // depth값이 작은 것(z가 작아서 앞에 있는 것)을 그린다.
	psoDesc.DepthStencilState.StencilEnable = false;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 생성했던 리소스와 동일한 포맷을 이용한다.

	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX; // 지 혼자 0으로 초기회 된다.
	
	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf())))) {
		__debugbreak();
	}

	return true;
}

void BasicMeshObject::CleanUpMesh()
{
	if (m_pTriGroupList) {
		for (DWORD i = 0; i < m_dwTriGroupCount; i++) {
			if (m_pTriGroupList[i].pTexHandle) {
				m_pRenderer->DeleteTexture(m_pTriGroupList[i].pTexHandle);
				m_pTriGroupList[i].pTexHandle = nullptr;
			}
		}
		delete[] m_pTriGroupList;
		m_pTriGroupList = nullptr;
	}
	CleanupSharedResources();
}

BasicMeshObject::BasicMeshObject()
	:m_pRenderer(nullptr), 
	m_pVertexBuffer(nullptr), m_VertexBufferView{},
	m_pTriGroupList(nullptr), m_dwTriGroupCount(0), m_dwMaxTriGroupCount(0)
{
}

BasicMeshObject::~BasicMeshObject()
{
	CleanUpMesh();
}

/*
bool BasicMeshObject::CreateMesh()
{
	bool bResult = false;

	// Default Buffer에 Vertex 정보를 올려보는 것
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// 대충 찍어보자.
	BasicVertex Vertices[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
	};

	uint16_t Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	const UINT vertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), static_cast<DWORD>(_countof(Vertices)), &m_VertexBufferView, &m_pVertexBuffer, Vertices))) {
		__debugbreak();
		goto RETURN;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer(static_cast<DWORD>(_countof(Indices)), &m_IndexBufferView, &m_pIndexBuffer, Indices))) {
		__debugbreak();
		goto RETURN;
	}

	bResult = true;
RETURN:
	return bResult;
}

bool BasicMeshObject::CreateDescriptorTable()
{
	bool bResult = false;
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	// SRV으로 사이즈를 구하고
	m_srvDescriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// descriptor heap을 생성한다.
	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = DESCRIPTOR_COUNT_FOR_DRAW;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FAILED(pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pDescriptorHeap.GetAddressOf())))) {
		__debugbreak();
		goto RETURN;
	}

	bResult = true;
RETURN:
	return bResult;
}

bool BasicMeshObject::CreateMesh_UploadHeap()
{
	// 지금은 임시로 점을 임의로 몇개 찍어서 그려보는 것이다.
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	// 대충 찍어보자.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.33f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.33f, -0.33f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.33f, -0.33f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(Vertices);

	// vertex buffer도 똑같이 ID3D12Resoure로 만든다.
	// 근데 이 버퍼를 시스템 메모리(램)에 만들수 있고, GPU 메모리에도 만들 수 있다.
	// GPU 메모리에 올리는 것이 더 빠르다. 
	// (D3D12는 둘 중에 어느 메모리에 올라가게 할 것인지 정할 수 있다.)
	// 하지만 GPU 메모리에 올리는 것은 매우 까다롭다.
	// 그래서 지금은 시스템 메모리에 올리기로 한다.
	// 시스템 메모리에 올라가는 Resource의 한계는
	// CB, VB, IB 같이 하드웨어 입장에서 사이즈가 크지 않고 + 많이 로드하지 않는 친구들에 대해서만 허용된다.
	// (나중에는 GPU 메모리에 올려야 한다.)

	D3D12_HEAP_PROPERTIES default_HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	if (FAILED(pD3DDevice->CreateCommittedResource(
		&default_HeapProps, D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pVertexBuffer.GetAddressOf())
	))) {
		__debugbreak();
	}
	// 위에 보면 heap type을 지정하는데
	// 1. DEFAULT는 GPU 메모리
	// 2. UPLOAD는 CPU 메모리
	// 3. READBACK은 AO(ambient occlusion) 같이 GPU가 그린 것을 다시 읽어와야 할 때
	// 4. CUSTOM 은 뭐... 이런것들을 프로그래머가 설정할 때
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_memory_pool 
	// 5. GPU_UPLOAD는 Resizable BAR(Base Address Register) 라고 CPU가 memory mapped IO를 이용해서
	// 시스템 메모리 처럼 데이터를 write 하면, 그게 PCI 버스를 타고 GPU에 write를 해준다는 것이다.
	// 내부적으로 Lock을 걸어서 비교적 프로그래머가 편하게 사용할 수 있다.
	// (위에 처럼 복잡하게 write가 되었는지 확인할 필요가 없게 된다.)
	// (이걸 지원하는 하드웨어가 있다면 GPU 메모리에 쉽게 접근할 수 있게 된다.)

	// 삼각형 정보를 Vertex Buffer에 넣는다.
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	// Map 함수를 호출하면, CPU에서 바로 사용할 수 있는 Address를 얻을 수 있다.
	// (Memory mapped IO나 PCI Bus로 GPU에게 전달되어야 하는데,)
	// (이 친구가 실제 메모리가 아니라 가상의 메모리일 가능성이 있다.)
	// Map 이 친구는 내부적으로 뭔가를 할 수도, 안 할 수도 있는 것이다.
	// https://megayuchi.com/2022/06/09/d3d12%EC%9D%98-map%EC%97%90-%EB%8C%80%ED%95%9C-%EA%B3%A0%EC%B0%B0/

	if (FAILED(m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)))) {
		__debugbreak();
	}
	memcpy(pVertexDataBegin, Vertices, sizeof(Vertices));
	m_pVertexBuffer->Unmap(0, nullptr);

	// vertex buffer view도 초기화 한다.
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(BasicVertex);
	m_VertexBufferView.SizeInBytes = vertexBufferSize;

	return true;
}

bool BasicMeshObject::CreateMesh_DefaultHeap()
{
	bool bResult = false;

	// Default Buffer에 Vertex 정보를 올려보는 것
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// 대충 찍어보자.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.33f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.33f, -0.33f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.33f, -0.33f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), static_cast<DWORD>(_countof(Vertices)), &m_VertexBufferView, &m_pVertexBuffer, Vertices))) {
		__debugbreak();
		goto RETURN;
	}

	bResult = true;
RETURN:
	return bResult;
}

bool BasicMeshObject::CreateMesh_WithIndex()
{
	bool bResult = false;

	// Default Buffer에 Vertex와 Index를 올리는 것
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// vertex buffer.
	BasicVertex Vertices[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.5f, 0.5f, 1.0f } }
	};
	// index buffer
	uint16_t Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	const UINT vertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), static_cast<DWORD>(_countof(Vertices)), &m_VertexBufferView, &m_pVertexBuffer, Vertices))) {
		__debugbreak();
		goto RETURN;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer(static_cast<DWORD>(_countof(Indices)), &m_IndexBufferView, &m_pIndexBuffer, Indices))) {
		__debugbreak();
		goto RETURN;
	}

	bResult = true;
RETURN:
	return bResult;
}

bool BasicMeshObject::CreateMesh_WithTexture()
{
	bool bResult = false;

	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();
	// uv 좌표도 함께 가진 Vertex buffer를 만든다.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.5f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
	};

	const UINT  VertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices),
		&m_VertexBufferView, &m_pVertexBuffer, Vertices))) {
		__debugbreak();
		goto RETURN;
	}

	{
		// 입힐 텍스쳐를 16x16으로 생성한다.
		const UINT texWidth = 16;
		const UINT texHeight = 16;
		// 1픽셀당 32바이트 포멧으로 하고
		DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 메모리를 할당한 다음
		BYTE* pImage = static_cast<BYTE*>(malloc(texWidth * texHeight * sizeof(uint32_t)));
		memset(pImage, 0, texWidth * texHeight * sizeof(uint32_t));

		// 셰이더에서 Vertex Color에 Texture Color가 곱해질 것이다.
		BOOL bFirstColorIsWhite = TRUE;

		for (UINT y = 0; y < texHeight; y++) {
			for (UINT x = 0; x < texWidth; x++) {
				RGBA* pDest = reinterpret_cast<RGBA*>(pImage + (y * texWidth + x) * sizeof(uint32_t));

				pDest->r = rand() % 255;
				pDest->g = rand() % 255;
				pDest->b = rand() % 255;
				// TRUE 니까 흰검흰검 순으로 텍스쳐가 그려진다.
				if ((bFirstColorIsWhite + x) % 2) {
					pDest->r = 255;
					pDest->g = 255;
					pDest->b = 255;
				}
				else {
					pDest->r = 0;
					pDest->g = 0;
					pDest->b = 0;
				}
				pDest->a = 255;
			}
			bFirstColorIsWhite++;
			bFirstColorIsWhite %= 2;
		}

		pResourceManager->CreateTexture(&m_pTexResource, texWidth, texHeight, texFormat, pImage);

		free(pImage);

		// Texture를 Shader로 넘기기 위한 논리 구조인 Descriptor Table을 생성한다.
		CreateDescriptorTable();

		// Texture Resource의 정보로 Shader Resource View를 생성한다.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texFormat;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// GPU가 참조할 리소스로서 Shader Resource View를 클래스가 가지고 있는 Descriptor Heap에 0번 offset에 생성한다.
		CD3DX12_CPU_DESCRIPTOR_HANDLE srv(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(BASIC_MESH_DESCRIPTOR_INDEX::TEX), m_srvDescriptorSize);
		pD3DDevice->CreateShaderResourceView(m_pTexResource.Get(), &srvDesc, srv);
	}

	bResult = true;
RETURN:
	return bResult;
}

bool BasicMeshObject::CreateMesh_WithCB()
{
	bool bResult = CreateMesh_WithTexture();
	if (!bResult) {
		__debugbreak();
		goto RETURN;
	}

	// Constant Buffer를 만든다.
	{
		Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
		D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

		// CB는 하드웨어 성능상의 이유로 256-bytes aligned이여야 한다.
		const UINT constantBufferSize = static_cast<UINT>(AlignConstantBufferSize(sizeof(CONSTANT_BUFFER_DEFAULT)));

		D3D12_HEAP_PROPERTIES heapProps_Default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC cbDesc_Size = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
		// Upload Buffer로 만들어줘서, 상시 업데이트가 가능하도록 만들자.
		HRESULT hr = pD3DDevice->CreateCommittedResource(
			&heapProps_Default,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc_Size,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(m_pConstantBuffer.GetAddressOf())
		);
		if (FAILED(hr)) {
			__debugbreak();
		}

		// Constant Buffer View도 생성해준다.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		// heap 위의 위치를 지정해주고
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbv(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(BASIC_MESH_DESCRIPTOR_INDEX::CBV), m_srvDescriptorSize);
		pD3DDevice->CreateConstantBufferView(&cbvDesc, cbv);

		// 초기 값을 지정해주고, 지속적으로 업데이트를 해주기 위해 Unmap을 하지 않는다. 
		CD3DX12_RANGE writeRange(0, 0);
		hr = m_pConstantBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSysConstBufferDefault));
		if (FAILED(hr)) {
			__debugbreak();
		}
		m_pSysConstBufferDefault->offset.x = 0.f;
		m_pSysConstBufferDefault->offset.y = 0.f;
	}
	bResult = true;
RETURN:
	return bResult;
}

*/