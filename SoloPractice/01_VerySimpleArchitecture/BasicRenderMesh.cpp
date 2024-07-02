#include "pch.h"
#include "BasicRenderMesh.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "D3DUtil.h"
#include "ConstantBufferPool.h"
#include "DescriptorPool.h"


Microsoft::WRL::ComPtr<ID3D12RootSignature> BasicRenderMesh::m_pRootSignature = nullptr;

bool BasicRenderMesh::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;

	bool bResult = InitCommonResources();

	// �ϴ��� �ν��Ͻ����� PSO�� ������ �Ѵ�.
	bResult = InitPipelineState();
	return bResult;
}

void BasicRenderMesh::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld)
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	// Renderer�� �����ϴ� Pool
	ConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE::DEFAULT);
	DescriptorPool* pDescriptorPool = m_pRenderer->INL_DescriptorPool();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pPoolDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	// �ϴ��� subRenderGeoCount + 1��ŭ pool���� �Ҵ� �޴´�. CB �Ѱ� + SubRenderGeoCount ��ŭ ���� Texture
	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, m_subRenderGeoCount + 1)) {
		__debugbreak();
	}

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
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(cpuDescriptorTable, static_cast<INT>(BASIC_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ::CBV), srvDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, dest, pCB->cbvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // CPU �ڵ忡���� CPU Handle�� write�� �����ϴ�.

	// pool���� allocation �޾ұ� ������ ������ Heap���� �ֱ⿡ 
	dest.Offset(1, srvDescriptorSize); // �̷��� ���� �ڵ��� offset�� �̿��� ���� �ڸ��� ���ϸ� �ȴ�.

	for (UINT i = 0; i < m_subRenderGeoCount; i++) 
	{
		SubRenderGeometry* pSubRenderGeo = subRenderGeometries[i];
		TEXTURE_HANDLE* pTexHandle = pSubRenderGeo->pTexHandle;
		if (pSubRenderGeo)
		{
			pD3DDevice->CopyDescriptorsSimple(1, dest, pTexHandle->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else 
		{

		}
	}
}

void BasicRenderMesh::CreateRenderAssets(MeshData** _ppMeshData, const UINT _meshDataCount)
{
	if (_meshDataCount > MAX_SUB_RENDER_GEO_COUNT) {
		__debugbreak();
		return;
	}

	m_subRenderGeoCount = _meshDataCount;

	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();
	
	for (UINT i = 0; i < m_subRenderGeoCount; i++) {
		subRenderGeometries[i] = new SubRenderGeometry;
		MeshData* pCurMeshData = *(_ppMeshData + i);

		// Vertex Buffer ���� �����Ѵ�.
		if (FAILED(pResourceManager->CreateVertexBuffer(
			sizeof(BasicVertex), pCurMeshData->Vertices.size(), 
			&(subRenderGeometries[i]->m_VertexBufferView),
			&(subRenderGeometries[i]->m_pVertexBuffer), 
			(void*)pCurMeshData->Vertices.data()
		)))
		{
			__debugbreak();
		}

		// Index Buffer�� �����Ѵ�.
		if (FAILED(pResourceManager->CreateIndexBuffer(
			pCurMeshData->Indices32.size(),
			&(subRenderGeometries[i]->m_IndexBufferView),
			&(subRenderGeometries[i]->m_pIndexBuffer),
			(void*)(pCurMeshData->GetIndices16().data())
		)))
		{
			__debugbreak();
		}
		subRenderGeometries[i]->indexCount = pCurMeshData->Indices32.size();
		subRenderGeometries[i]->startIndexLocation = 0;
		subRenderGeometries[i]->baseVertexLocation = 0;
	}
}

void BasicRenderMesh::BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex)
{
	if (m_subRenderGeoCount <= _subRenderAssetIndex) 
	{
		__debugbreak();
	}
	subRenderGeometries[_subRenderAssetIndex]->pTexHandle = _pTexHandle;
}

bool BasicRenderMesh::InitCommonResources()
{
	// root signature�� �̱������� ����Ѵ�.
	if (m_dwInitRefCount > 0) {
		goto EXIST;
	}

	InitRootSignature();

EXIST:
	m_dwInitRefCount++;
	return true;
}

void BasicRenderMesh::CleanupSharedResources()
{
	if (!m_dwInitRefCount) {
		return;
	}

	DWORD refCount = --m_dwInitRefCount;
	// ���� �����ϴ� Object�� �ִٸ� �������� �ʱ�
	if (!refCount) {
		// ��״� data section�� �ִ� �ֵ��̶� ���� ������ ����� �Ѵ�.
		m_pRootSignature = nullptr;
		//m_pPipelineState = nullptr;
	}
}

bool BasicRenderMesh::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	// �ϴ� CB �ϳ� SRV �ϳ� �Ѿ�°ŷ� �Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;


	CD3DX12_DESCRIPTOR_RANGE rangePerObj[2] = {};
	rangePerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 :  Object ���� �ѱ�� Constant Buffer View
	rangePerObj[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 :  �ϴ��� Object ���� �ѱ�� SRV(texture)

	// table 0���� 1���� �����Ѵ�.
	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangePerObj), rangePerObj, D3D12_SHADER_VISIBILITY_ALL);


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

bool BasicRenderMesh::InitPipelineState()
{
	//D3D12PSOCache* pD3DPSOCache = m_pRenderer->INL_GetD3D12PSOCache();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState = nullptr;
	std::string psoKey = g_PSOKeys[(UINT)PSO_KEYS_INDEX::DEFAULT_FILL];
	pPipelineState = m_pRenderer->GetPSO(psoKey);
	if (pPipelineState != nullptr) {
		m_pPipelineState = pPipelineState;
		goto RETURN;
	}
	else {
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

		m_pRenderer->CachePSO(psoKey, m_pPipelineState);
	}

RETURN:
	return true;
}

void BasicRenderMesh::CleanUpAssets()
{
	for (int i = 0; i < m_subRenderGeoCount; i++) {
		if (subRenderGeometries[i]) {
			delete subRenderGeometries[i];
			subRenderGeometries[i] = nullptr;
		}
	}
	CleanupSharedResources();
}

BasicRenderMesh::BasicRenderMesh()
	:m_pRenderer(nullptr), subRenderGeometries{nullptr, }, 
	m_subRenderGeoCount(0), m_pPipelineState(nullptr)
{
}

BasicRenderMesh::~BasicRenderMesh()
{
	CleanUpAssets();
}
