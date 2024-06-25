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
	// D3D12�� ���� �񵿱� API�̰�, GPU�� CPU�� Ÿ�Ӷ����� ����ȭ �Ǿ����� �ʴ�.
	// �׷��� draw�� �� ��, shader�� �Ѿ�� resource���� ���еǾ �����ϰ� �־�� �Ѵ�.
	// ���⼭�� descriptor table�� constant buffer�� ���Ѵ�.
	// �׷��� �� �Լ��� ȣ�� �� ������, CBV pool�� Descriptor pool���� �ϳ��� ���;� �Ѵ�.

	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	// Renderer�� �����ϴ� Pool
	ConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool();
	DescriptorPool* pDescriptorPool = m_pRenderer->INL_DescriptorPool();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pPoolDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	// ���⼭ ��� Texture�� CB�� �Ѿ�� ����� ����
	DWORD dwRequiredDescriptorCount = DESCRIPTOR_COUNT_PER_OBJ + (m_dwTriGroupCount * DESCRIPTOR_COUNT_PER_TRI_GROUP);
	// �� ������ŭ pool���� �Ҵ� �޴´�.
	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, dwRequiredDescriptorCount)) {
		__debugbreak();
	}

	// ���������δ� Ŀ�ٶ� Resource�� �������� ���̰�, Constant Buffer View�� pointer ������ �ϴ� ���̴�.
	CB_CONTAINER* pCB = pConstantBufferPool->Alloc();
	if (!pCB) {
		__debugbreak();
	}

	CONSTANT_BUFFER_DEFAULT* pConstantBufferDefault = reinterpret_cast<CONSTANT_BUFFER_DEFAULT*>(pCB->pSystemMemAddr);
	// CB ���� ������Ʈ �ϰ�
	XMMATRIX viewMat;
	XMMATRIX projMat;
	m_pRenderer->GetViewProjMatrix(&viewMat, &projMat);

	pConstantBufferDefault->matView = XMMatrixTranspose(viewMat);
	pConstantBufferDefault->matProj = XMMatrixTranspose(projMat);
	pConstantBufferDefault->matWorld = XMMatrixTranspose(*_pMatWorld);

	XMMATRIX wvpMat = (*_pMatWorld) * viewMat * projMat;

	pConstantBufferDefault->matWVP = XMMatrixTranspose(wvpMat);

	// �ʱ�ȭ �� �� ���� Texture�� ������Ʈ�� CB�� �ѱ�, Descriptor Table�� �����Ѵ�.

	// Object ���� �Ѿ�� CB�� CopyDescriptorSimple�� �̿��ؼ�, ���� Constant Buffer View�� ���� �Ҵ�� Descriptor Heap ��ġ�� �ִ� Descriptor�� �����Ѵ�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(cpuDescriptorTable, static_cast<INT>(BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ::CBV), srvDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, dest, pCB->cbvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // CPU �ڵ忡���� CPU Handle�� write�� �����ϴ�.

	// pool���� allocation �޾ұ� ������ ������ Heap���� �ֱ⿡ 
	dest.Offset(1, srvDescriptorSize); // �̷��� ���� �ڵ��� offset�� �̿��� ���� �ڸ��� ���ϸ� �ȴ�.

	// �׸���, Triangle ���� �Ѿ�� SRV(Texture)�� �����Ѵ�.
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


	// ��Ʈ �ñ״�ó�� �����ϰ�
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// pool�� �ִ� Heap���� �����Ѵ�.
	_pCommandList->SetDescriptorHeaps(1, pPoolDescriptorHeap.GetAddressOf());

	// �ϴ� ���̺� ��ȣ�� ���缭 0������ CBV�� �Ѱ��ش�. ������ 1���� �Ѱ��ش�.
	_pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

	// �׸��� Texture SRV�� �ٲٸ鼭 bind ���ְ�, �ﰢ���� �׸���.
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTriGroup(gpuDescriptorTable, DESCRIPTOR_COUNT_PER_OBJ, srvDescriptorSize);
	for (DWORD i = 0; i < m_dwTriGroupCount; i++) 
	{
		// ���̺� ��ȣ 1���� �־��ָ鼭 �ﰢ���� �׸���.
		_pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTriGroup);

		INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList + i;
		_pCommandList->IASetIndexBuffer(&(pTriGroup->indexBufferView));
		_pCommandList->DrawIndexedInstanced(pTriGroup->dwTriCount * 3, 1, 0, 0, 0);

		// ���� �ؽ��ĸ� bind ���ش�.
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
	// �ϴ� Vertex Buffer ���� �����Ѵ�.
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
	// �ε����� �ﰢ�� �׷���� �����.
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
	// root signature�� PSO�� �̱������� ����Ѵ�.
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
	// ���� �����ϴ� Object�� �ִٸ� �������� �ʱ�
	if (!refCount) {
		// ��״� data section�� �ִ� �ֵ��̶� ���� ������ ����� �Ѵ�.
		m_pRootSignature = nullptr;
		m_pPipelineState = nullptr;
	}
}

bool BasicMeshObject::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();


	// �߰��� Serialize�� ������ blob�� �����
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

	// CB�� Texture�� �Ѿ�°� ǥ������ root signature�� �����.
	// GPU�� �Ȱ��� �޾Ƶ�������, CPU������ �ǹ������� ���� �ٸ���,
	// ������ �ѱ�� �͵�, ��û�ϴ� �͵� �ٸ��� ������ Entry�� �и��ؼ� ���ش�.
	CD3DX12_DESCRIPTOR_RANGE rangePerObj[1] = {};
	rangePerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 :  Object ���� �ѱ�� Constant Buffer View

	CD3DX12_DESCRIPTOR_RANGE rangePerTri[1] = {};
	rangePerTri[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 : �ﰢ�� ���� �ѱ�� texture
	
	// table 0���� 1���� �����Ѵ�.
	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangePerObj), rangePerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangePerTri), rangePerTri, D3D12_SHADER_VISIBILITY_ALL);


	// Texture Sample�� �Ҷ� ����ϴ� Sampler��
	// static sampler�� Root Signature�� �Բ� �Ѱ��ش�.
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0); // s0 : sampler
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// ���� Table Entry�� Sampler�� �Ѱ��ָ鼭 Root Signature�� �����.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	// Serialize �� ����
	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignatureBlob.GetAddressOf(), pErrorBlob.GetAddressOf()))) {
		__debugbreak();
	}
	// Root signature�� �����.
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
	// ���̴��� ������ �Ҷ�, ����ȭ�� ���ϰ� ������ϱ� ���ϵ��� �ϴ� ���̴�.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlas = 0;
#endif
	HRESULT hr;

	// vertex shader�� �������ϰ�
	hr = D3DCompileFromFile(L".\\Shaders\\Simple.hlsl", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, pVertexShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		// �޸� : �� �������� D3DCompiler_47.dll �ε� ������ ���.
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}
	// pixel shader�� ������ �Ѵ�.
	hr = D3DCompileFromFile(L".\\Shaders\\Simple.hlsl", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, pPixelShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		// �޸� : �� �������� D3DCompiler_47.dll �ε� ������ ���.
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}

	// input layout�� �����ϰ�
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) + sizeof(XMFLOAT4),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Pipeline State Object (PSO)�� �����.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = true; // ���� depth ���۸� ����Ѵ�.
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // depth���� ���� ��(z�� �۾Ƽ� �տ� �ִ� ��)�� �׸���.
	psoDesc.DepthStencilState.StencilEnable = false;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // �����ߴ� ���ҽ��� ������ ������ �̿��Ѵ�.

	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX; // �� ȥ�� 0���� �ʱ�ȸ �ȴ�.
	
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

	// Default Buffer�� Vertex ������ �÷����� ��
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// ���� ����.
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
	// SRV���� ����� ���ϰ�
	m_srvDescriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// descriptor heap�� �����Ѵ�.
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
	// ������ �ӽ÷� ���� ���Ƿ� � �� �׷����� ���̴�.
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	// ���� ����.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 0.33f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.33f, -0.33f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.33f, -0.33f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT vertexBufferSize = sizeof(Vertices);

	// vertex buffer�� �Ȱ��� ID3D12Resoure�� �����.
	// �ٵ� �� ���۸� �ý��� �޸�(��)�� ����� �ְ�, GPU �޸𸮿��� ���� �� �ִ�.
	// GPU �޸𸮿� �ø��� ���� �� ������. 
	// (D3D12�� �� �߿� ��� �޸𸮿� �ö󰡰� �� ������ ���� �� �ִ�.)
	// ������ GPU �޸𸮿� �ø��� ���� �ſ� ��ٷӴ�.
	// �׷��� ������ �ý��� �޸𸮿� �ø���� �Ѵ�.
	// �ý��� �޸𸮿� �ö󰡴� Resource�� �Ѱ��
	// CB, VB, IB ���� �ϵ���� ���忡�� ����� ũ�� �ʰ� + ���� �ε����� �ʴ� ģ���鿡 ���ؼ��� ���ȴ�.
	// (���߿��� GPU �޸𸮿� �÷��� �Ѵ�.)

	D3D12_HEAP_PROPERTIES default_HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	if (FAILED(pD3DDevice->CreateCommittedResource(
		&default_HeapProps, D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_pVertexBuffer.GetAddressOf())
	))) {
		__debugbreak();
	}
	// ���� ���� heap type�� �����ϴµ�
	// 1. DEFAULT�� GPU �޸�
	// 2. UPLOAD�� CPU �޸�
	// 3. READBACK�� AO(ambient occlusion) ���� GPU�� �׸� ���� �ٽ� �о�;� �� ��
	// 4. CUSTOM �� ��... �̷��͵��� ���α׷��Ӱ� ������ ��
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_memory_pool 
	// 5. GPU_UPLOAD�� Resizable BAR(Base Address Register) ��� CPU�� memory mapped IO�� �̿��ؼ�
	// �ý��� �޸� ó�� �����͸� write �ϸ�, �װ� PCI ������ Ÿ�� GPU�� write�� ���شٴ� ���̴�.
	// ���������� Lock�� �ɾ ���� ���α׷��Ӱ� ���ϰ� ����� �� �ִ�.
	// (���� ó�� �����ϰ� write�� �Ǿ����� Ȯ���� �ʿ䰡 ���� �ȴ�.)
	// (�̰� �����ϴ� �ϵ��� �ִٸ� GPU �޸𸮿� ���� ������ �� �ְ� �ȴ�.)

	// �ﰢ�� ������ Vertex Buffer�� �ִ´�.
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	// Map �Լ��� ȣ���ϸ�, CPU���� �ٷ� ����� �� �ִ� Address�� ���� �� �ִ�.
	// (Memory mapped IO�� PCI Bus�� GPU���� ���޵Ǿ�� �ϴµ�,)
	// (�� ģ���� ���� �޸𸮰� �ƴ϶� ������ �޸��� ���ɼ��� �ִ�.)
	// Map �� ģ���� ���������� ������ �� ����, �� �� ���� �ִ� ���̴�.
	// https://megayuchi.com/2022/06/09/d3d12%EC%9D%98-map%EC%97%90-%EB%8C%80%ED%95%9C-%EA%B3%A0%EC%B0%B0/

	if (FAILED(m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)))) {
		__debugbreak();
	}
	memcpy(pVertexDataBegin, Vertices, sizeof(Vertices));
	m_pVertexBuffer->Unmap(0, nullptr);

	// vertex buffer view�� �ʱ�ȭ �Ѵ�.
	m_VertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes = sizeof(BasicVertex);
	m_VertexBufferView.SizeInBytes = vertexBufferSize;

	return true;
}

bool BasicMeshObject::CreateMesh_DefaultHeap()
{
	bool bResult = false;

	// Default Buffer�� Vertex ������ �÷����� ��
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// ���� ����.
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

	// Default Buffer�� Vertex�� Index�� �ø��� ��
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
	// uv ��ǥ�� �Բ� ���� Vertex buffer�� �����.
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
		// ���� �ؽ��ĸ� 16x16���� �����Ѵ�.
		const UINT texWidth = 16;
		const UINT texHeight = 16;
		// 1�ȼ��� 32����Ʈ �������� �ϰ�
		DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		// �޸𸮸� �Ҵ��� ����
		BYTE* pImage = static_cast<BYTE*>(malloc(texWidth * texHeight * sizeof(uint32_t)));
		memset(pImage, 0, texWidth * texHeight * sizeof(uint32_t));

		// ���̴����� Vertex Color�� Texture Color�� ������ ���̴�.
		BOOL bFirstColorIsWhite = TRUE;

		for (UINT y = 0; y < texHeight; y++) {
			for (UINT x = 0; x < texWidth; x++) {
				RGBA* pDest = reinterpret_cast<RGBA*>(pImage + (y * texWidth + x) * sizeof(uint32_t));

				pDest->r = rand() % 255;
				pDest->g = rand() % 255;
				pDest->b = rand() % 255;
				// TRUE �ϱ� ������ ������ �ؽ��İ� �׷�����.
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

		// Texture�� Shader�� �ѱ�� ���� �� ������ Descriptor Table�� �����Ѵ�.
		CreateDescriptorTable();

		// Texture Resource�� ������ Shader Resource View�� �����Ѵ�.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texFormat;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// GPU�� ������ ���ҽ��μ� Shader Resource View�� Ŭ������ ������ �ִ� Descriptor Heap�� 0�� offset�� �����Ѵ�.
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

	// Constant Buffer�� �����.
	{
		Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
		D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

		// CB�� �ϵ���� ���ɻ��� ������ 256-bytes aligned�̿��� �Ѵ�.
		const UINT constantBufferSize = static_cast<UINT>(AlignConstantBufferSize(sizeof(CONSTANT_BUFFER_DEFAULT)));

		D3D12_HEAP_PROPERTIES heapProps_Default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC cbDesc_Size = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
		// Upload Buffer�� ������༭, ��� ������Ʈ�� �����ϵ��� ������.
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

		// Constant Buffer View�� �������ش�.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		// heap ���� ��ġ�� �������ְ�
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbv(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(BASIC_MESH_DESCRIPTOR_INDEX::CBV), m_srvDescriptorSize);
		pD3DDevice->CreateConstantBufferView(&cbvDesc, cbv);

		// �ʱ� ���� �������ְ�, ���������� ������Ʈ�� ���ֱ� ���� Unmap�� ���� �ʴ´�. 
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