// BasicMeshObject.cpp from "megayuchi"

#include "pch.h"
#include "BasicMeshObject.h"
#include "D3D12ResourceManager.h"
#include "ShaderManager.h"
#include "SimpleConstantBufferPool.h"
#include "SingleDescriptorAllocator.h"
#include "DescriptorPool.h"
#include "D3D12Renderer.h"
#include "RayTracingManager.h"

SHADER_HANDLE* BasicMeshObject::m_pVS = nullptr;
SHADER_HANDLE* BasicMeshObject::m_pPS = nullptr;
D3D12RootSignature_ptr BasicMeshObject::m_pRootSignature = nullptr;
D3D12PipelineState_ptr BasicMeshObject::m_pPipelineState = nullptr;
ULONG BasicMeshObject::m_ulInitRefCount;

bool BasicMeshObject::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;
	bool bResult = InitCommonResources();

	return bResult;
}

void BasicMeshObject::Draw(D3D12GraphicsCommandList_raw _pCommandList, const XMMATRIX* _pMatWorld)
{

}

bool BasicMeshObject::BeginCreateMesh(const BasicVertex* _pVertexList, ULONG _ulVertexNum, ULONG _ulTriGroupCount)
{
	return false;
}

bool BasicMeshObject::InsertIndexedTriList(const uint16_t* _pIndexList, ULONG _ulTriCount, const WCHAR* _wchTexFileName)
{
	return false;
}

void BasicMeshObject::EndCreateMesh()
{
}

void* BasicMeshObject::CreateBLAS()
{
	return nullptr;
}

void BasicMeshObject::DeleteBLAS(void* _pBlasHandle)
{
}

bool BasicMeshObject::InitCommonResources()
{
	if (m_ulInitRefCount > 0) {
		m_ulInitRefCount++;
		return true;
	}

	InitRootSignature();
	InitPipelineState();

	m_ulInitRefCount++;
	return true;
}

void BasicMeshObject::CleanupSharedResources()
{
	ShaderManager* pShaderManager = m_pRenderer->INL_GetShaderManager();

	if (!m_ulInitRefCount) 
		return;

	ULONG ref_count = --m_ulInitRefCount;
	if (ref_count <= 0) {
		if (m_pVS) {
			pShaderManager->ReleaseShader(m_pVS);
			m_pVS = nullptr;
		}
		if (m_pPS) {
			pShaderManager->ReleaseShader(m_pPS);
			m_pPS = nullptr;
		}
		m_pRootSignature = nullptr;
		m_pPipelineState = nullptr;
	}
}

bool BasicMeshObject::InitRootSignature()
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

	// Object - CBV - RootParam(0)
	// {
	//   TriGrup 0 - SRV[0] - RootParam(1) - Draw()
	//   TriGrup 1 - SRV[1] - RootParam(1) - Draw()
	//   TriGrup 2 - SRV[2] - RootParam(1) - Draw()
	//   TriGrup 3 - SRV[3] - RootParam(1) - Draw()
	//   TriGrup 4 - SRV[4] - RootParam(1) - Draw()
	//   TriGrup 5 - SRV[5] - RootParam(1) - Draw()
	// }

	CD3DX12_DESCRIPTOR_RANGE rangesPerObj[1] = {};
	rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 : Constant Buffer View per Object

	CD3DX12_DESCRIPTOR_RANGE rangesPerTriGroup[1] = {};
	rangesPerTriGroup[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 : Shader Resource View per Triangle Group

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangesPerObj), rangesPerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangesPerTriGroup), rangesPerTriGroup, D3D12_SHADER_VISIBILITY_ALL);

	// default sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// 깡통 Root Signature를 생성한다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignatureBlob, &pErrorBlob);
	if(FAILED(hr)) {
		if (pErrorBlob) {
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
		return false;
	}

	hr = pD3DDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	if(FAILED(hr)) {
		OutputDebugStringA("Failed to create root signature.\n");
		__debugbreak();
		return false;
	}

	return true;
}

bool BasicMeshObject::InitPipelineState()
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	ShaderManager* pShaderManager = m_pRenderer->INL_GetShaderManager();

	m_pVS = pShaderManager->CreateShaderDXC(L"shBasicMesh.hlsl", L"VSMain", L"vs_6_0", 0);
	if (!m_pVS) {
		OutputDebugStringA("Failed to create vertex shader.\n");
		__debugbreak();
		return false;
	}
	m_pPS = pShaderManager->CreateShaderDXC(L"shBasicMesh.hlsl", L"PSMain", L"ps_6_0", 0);
	if (!m_pPS) {
		OutputDebugStringA("Failed to create pixel shader.\n");
		__debugbreak();
		return false;
	}

	// vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 6, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 9, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, sizeof(float) * 13,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Graphics Pipeline State Object (PSO) 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_pVS->pCodeBuffer, m_pVS->ullCodeSize);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pPS->pCodeBuffer, m_pPS->ullCodeSize);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create graphics pipeline state.\n");
		__debugbreak();
		return false;
	}

	return true;
}

void BasicMeshObject::DeleteTriGroup(INDEXED_TRI_GROUP* _pTriGroup)
{
	_pTriGroup->pIndexBuffer = nullptr;
	m_pRenderer->DeleteTexture(_pTriGroup->pTexHandle);
}

void BasicMeshObject::CleanUp()
{
	if(m_pTriGroupList.size() > 0) {
		for (auto& triGroup : m_pTriGroupList) {
			DeleteTriGroup(triGroup.get());
		}
		m_pTriGroupList.clear();
	}
	if(m_pVertexBuffer) {
		m_pVertexBuffer = nullptr;
	}
	CleanupSharedResources();
}

BasicMeshObject::BasicMeshObject()
{
}

BasicMeshObject::~BasicMeshObject()
{
	CleanUp();
}
