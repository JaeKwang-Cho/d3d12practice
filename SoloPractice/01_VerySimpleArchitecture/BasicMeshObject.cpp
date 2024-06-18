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
	// root signature�� �����ϰ�
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// PSO�� �����ϰ�
	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	// primitive topology type�� �����ϰ�
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// IA�� Vertex ������ bind ���ְ�
	_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	_pCommandList->DrawInstanced(3, 1, 0, 0);
}

bool BasicMeshObject::CreateMesh_UploadHeap()
{
	// ������ �ӽ÷� ���� ���Ƿ� � �� �׷����� ���̴�.
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
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
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
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

	DWORD refCount = m_dwInitRefCount - 1;
	// ���� �����ϴ� Object�� �ִٸ� �������� �ʱ�
	if (!refCount) {
		// ��״� data section�� �ִ� �ֵ��̶� ���� ������ ����� �Ѵ�.
		m_pRootSignature = nullptr;
		m_pPipelineState = nullptr;
	}
}

bool BasicMeshObject::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	// ���� root signature�� �����.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// �߰��� Serialize�� ������ blob�� �����
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

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
	Microsoft::WRL::ComPtr<ID3D12Device5> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

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
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Pipeline State Object (PSO)�� �����.
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
	psoDesc.SampleMask = UINT_MAX; // �� ȥ�� 0���� �ʱ�ȸ �ȴ�.
	
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

