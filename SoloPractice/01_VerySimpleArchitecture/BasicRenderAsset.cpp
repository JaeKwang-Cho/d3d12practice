#include "pch.h"
#include "BasicRenderAsset.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "D3DUtil.h"
#include "ConstantBufferPool.h"
#include "DescriptorPool.h"


Microsoft::WRL::ComPtr<ID3D12RootSignature> BasicRenderAsset::m_pRootSignature = nullptr;

bool BasicRenderAsset::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;

	bool bResult = InitCommonResources();

	// �ϴ��� �ν��Ͻ����� PSO�� ������ �Ѵ�.
	bResult = InitPipelineState();
	return bResult;
}

void BasicRenderAsset::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _ppMatWorld)
{
}

void BasicRenderAsset::CreateRenderAssets(const MeshData** _ppMeshData, const UINT _meshDataCount)
{
	if (_meshDataCount > MAX_SUB_RENDER_GEO_COUNT) {
		__debugbreak();
		return;
	}

	// todo : ���� �ؾ���
}

void BasicRenderAsset::BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex)
{
}

bool BasicRenderAsset::InitCommonResources()
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

void BasicRenderAsset::CleanupSharedResources()
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

bool BasicRenderAsset::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	// �ϴ� CB �ϳ� SRV �ϳ� �Ѿ�°ŷ� �Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;


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

bool BasicRenderAsset::InitPipelineState()
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

void BasicRenderAsset::CleanUpAssets()
{

	CleanupSharedResources();
}

BasicRenderAsset::BasicRenderAsset()
	:m_pRenderer(nullptr), subRenderGeometries{}, 
	m_subRenderGeoCount(0), m_pPipelineState(nullptr)
{
}

BasicRenderAsset::~BasicRenderAsset()
{
	CleanUpAssets();
}
