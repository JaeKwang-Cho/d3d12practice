// BasicMeshObject.cpp from "megayuchi"

#include "pch.h"
#include "BasicMeshObject.h"
#include "typedef.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"

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

void BasicMeshObject::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _pCommandList)
{
	// root signature를 설정하고
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// PSO를 설정하고
	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	// primitive topology type을 설정하고
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// IA에 Vertex 정보를 bind 해주고
	_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	_pCommandList->DrawInstanced(3, 1, 0, 0);
}

bool BasicMeshObject::CreateMesh_UploadHeap()
{
	// 지금은 임시로 점을 임의로 몇개 찍어서 그려보는 것이다.
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
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
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
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

	DWORD refCount = m_dwInitRefCount - 1;
	// 아직 참조하는 Object가 있다면 삭제하지 않기
	if (!refCount) {
		// 얘네는 data section에 있는 애들이라 직접 해제를 해줘야 한다.
		m_pRootSignature = nullptr;
		m_pPipelineState = nullptr;
	}
}

bool BasicMeshObject::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	// 깡통 root signature를 만든다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 중간에 Serialize를 도와줄 blob을 만들고
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

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
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

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
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Pipeline State Object (PSO)를 만든다.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.StencilEnable = false;

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX; // 지 혼자 0으로 초기회 된다.
	
	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf())))) {
		__debugbreak();
	}

	return true;
}

void BasicMeshObject::CleanUpMesh()
{
	//m_pVertexBuffer = nullptr;
	CleanupSharedResources();
}

BasicMeshObject::BasicMeshObject()
	:m_pRenderer(nullptr), m_pVertexBuffer(nullptr), m_VertexBufferView{}
{
}

BasicMeshObject::~BasicMeshObject()
{
	CleanUpMesh();
}

