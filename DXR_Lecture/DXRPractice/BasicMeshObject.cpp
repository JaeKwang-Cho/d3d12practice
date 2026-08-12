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
	// 각각의 draw() 작업의 무결성을 보장하려면, draw() 작업마다 다른 영역의 descriptor table(shader visible)과 다른 영역의 CBV를 사용해아 한다.
	// 따라서 draw() 할 때 마다, CBV는 ConstantBufferPool에서 할당 받고, 렌더링용 descriptor table(shader visible)은 desriptor pool로 부터 할당 받는다.

	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();

	DescriptorPool* pDescriptorPool = m_pRenderer->INL_GetDescriptorPool();
	D3D12DescriptorHeap_raw pDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	SimpleConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE::DEFAULT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	ULONG ulRequiredDescriptorCount = DESCRIPTOR_COUNT_PER_OBJ + (m_ulTriGroupCount * DESCRIPTOR_COUNT_PER_TRI_GROUP);

	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, ulRequiredDescriptorCount)) {
		OutputDebugStringA("Failed to allocate descriptor table for BasicMeshObject::Draw().\n");
		__debugbreak();
		return;
	}

	// 각각의 draw()에 대해 독립적인 constant buffer를 할당 받는다.
	// 같은 resource 안에 다른 영역을 사용한다.
	CB_CONTAINER* pCBContainer = pConstantBufferPool->AllocCBContainer();
	if(!pCBContainer) {
		OutputDebugStringA("Failed to allocate constant buffer for BasicMeshObject::Draw().\n");
		__debugbreak();
		return;
	}
	CONSTANT_BUFFER_DEFAULT* pConstantBufferDefault = reinterpret_cast<CONSTANT_BUFFER_DEFAULT*>(pCBContainer->pSysMemAddress);

	// CB 내용 채우기 
	// - view/projMat
	m_pRenderer->INL_GetViewProjMatrix(pConstantBufferDefault->matView, pConstantBufferDefault->matProj);
	// - worldMat
	pConstantBufferDefault->matWorld = XMMatrixTranspose(*_pMatWorld);

	// Descriptor Table 구성
	// 이번에 사용할 CB의 descriptor를 렌더링용(shader visible) descriptor table에 복사한다.

	// per obj
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dest(cpuDescriptorTable, static_cast<UINT>(E_BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ::CBV), srvDescriptorSize);
	// cpu측 코드에서는 cpu descriptor handle에만 write가 가능하다.
	pD3DDevice->CopyDescriptorsSimple(1, Dest, pCBContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); 
	Dest.Offset(1, srvDescriptorSize);

	// per tri-group
	for(ULONG i = 0; i < m_ulTriGroupCount; i++) {
		INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList[i].get();
		TEXTURE_HANDLE* pTexHandle = pTriGroup->pTexHandle;
		if (pTexHandle) {
			// 마찬가지로 cpu측 코드에서는 cpu descriptor handle에만 write가 가능하다.
			pD3DDevice->CopyDescriptorsSimple(1, Dest, pTexHandle->srvCpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else {
			OutputDebugStringA("BasicMeshObject::Draw() - TriGroup has no texture.\n");
			__debugbreak();
		}
		Dest.Offset(1, srvDescriptorSize);
	}

	// set RootSignature
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	_pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);

	// ex) TriGroup이 3개일 때
	// OBJ마다 - CBV 1개
	// TriGroup마다 - SRV 1개 (총 3개)

	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

	// descriptor table을 RootParam(0)에 bind한다.
	_pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable); // RootParam(0) : CBV per Object
	// offset을 cbv 갯수만큼 이동시켜서 helper 생성자에 넣는다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTriGroup(gpuDescriptorTable, DESCRIPTOR_COUNT_PER_OBJ, srvDescriptorSize);
	for (ULONG i = 0; i < m_ulTriGroupCount; i++) {
		// descriptor table을 RootParam(1)에 bind한다. 
		_pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTriGroup);
		gpuDescriptorTableForTriGroup.Offset(1, srvDescriptorSize);

		INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList[i].get();
		_pCommandList->IASetIndexBuffer(&pTriGroup->indexBufferView);
		_pCommandList->DrawIndexedInstanced(pTriGroup->ulTriCount * 3, 1, 0, 0, 0);
	}

}

bool BasicMeshObject::BeginCreateMesh(const BasicVertex* _pVertexList, ULONG _ulVertexNum, ULONG _ulTriGroupCount)
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	if (_ulTriGroupCount > MAX_TRI_GROUP_COUNT_PER_OBJ) {
		OutputDebugStringA("BasicMeshObject::BeginCreateMesh() - Too many triangle groups.\n");
		__debugbreak();
		return false;
	}

	HRESULT hr = pResourceManager->CreateVertexBuffer(
		sizeof(BasicVertex), _ulVertexNum, &m_VertexBufferView, &m_pVertexBuffer, const_cast<void*>(reinterpret_cast<const void*>(_pVertexList)));
	if (FAILED(hr)) {
		OutputDebugStringA("BasicMeshObject::BeginCreateMesh() - Failed to create vertex buffer.\n");
		__debugbreak();
		return false;
	}

	m_ulMaxTriGroupCount = _ulTriGroupCount;
	m_pTriGroupList.resize(m_ulMaxTriGroupCount);
	m_ulVertexCount = _ulVertexNum;

	return true;
}

bool BasicMeshObject::InsertIndexedTriList(const uint16_t* _pIndexList, ULONG _ulTriCount, const WCHAR* _wchTexFileName)
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();

	D3D12Resource_raw pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};

	if (m_ulTriGroupCount >= m_ulMaxTriGroupCount) {
		OutputDebugStringA("BasicMeshObject::BeginCreateMesh() - Too many triangle groups.\n");
		__debugbreak();
		return false;
	}

	ULONG ulIndicesSize = _ulTriCount * 3 * sizeof(USHORT);
	ULONG ulAlignedIndexSize = (ulIndicesSize / 16 + ((ulIndicesSize % 16) ? 1 : 0)) * 16; // 16-bytes aligned size
	ULONG ulAlignedIndexNum = ulAlignedIndexSize / sizeof(USHORT);

	HRESULT hr = pResourceManager->CreateIndexBuffer(
		ulAlignedIndexNum, &IndexBufferView, &pIndexBuffer, const_cast<void*>(reinterpret_cast<const void*>(_pIndexList)), sizeof(USHORT) * _ulTriCount * 3);
	if (FAILED(hr)) {
		OutputDebugStringA("BasicMeshObject::InsertIndexedTriList() - Failed to create index buffer.\n");
		__debugbreak();
		return false;
	}

	INDEXED_TRI_GROUP* pTriGroup = m_pTriGroupList[m_ulTriGroupCount].get();
	pTriGroup->pIndexBuffer = pIndexBuffer;
	pTriGroup->indexBufferView = IndexBufferView;
	pTriGroup->ulTriCount = _ulTriCount;
	pTriGroup->ulAlignedIndexCount = ulAlignedIndexNum;
	pTriGroup->pTexHandle = reinterpret_cast<TEXTURE_HANDLE*>(m_pRenderer->CreateTextureFromFile(_wchTexFileName));

	m_ulTriGroupCount++;

	return true;
}

void BasicMeshObject::EndCreateMesh()
{
}

void* BasicMeshObject::CreateBLAS()
{
	BLAS_INSTANCE* pBlasInstance = nullptr;
	RayTracingManager* pRayTracingManager = m_pRenderer->INL_GetRayTracingManager();

	std::vector<BLAS_BUILD_TRIGROUP_INFO> buildInfoList(m_ulTriGroupCount);

	DWORD dwBuildInfoCount = 0;
	for (DWORD i = 0; i < m_ulTriGroupCount; i++)
	{
		buildInfoList[dwBuildInfoCount].pIndexBuffer = m_pTriGroupList[i]->pIndexBuffer.Get();
		buildInfoList[dwBuildInfoCount].bNotOpaque = FALSE;
		buildInfoList[dwBuildInfoCount].ulIndexNum = m_pTriGroupList[i]->ulAlignedIndexCount;
		buildInfoList[dwBuildInfoCount].pDiffuseTexHandle = m_pTriGroupList[i]->pTexHandle;
		dwBuildInfoCount++;
	}
	pBlasInstance = pRayTracingManager->AllocBLAS(m_pVertexBuffer.Get(), sizeof(BasicVertex), m_ulVertexCount, buildInfoList.data(), dwBuildInfoCount, true);

	return reinterpret_cast<void*>(pBlasInstance);
}

void BasicMeshObject::DeleteBLAS(void* _pBlasHandle)
{
	RayTracingManager* pRayTracingManager = m_pRenderer->INL_GetRayTracingManager();
	BLAS_INSTANCE* pBlasInstance = reinterpret_cast<BLAS_INSTANCE*>(_pBlasHandle);
	pRayTracingManager->FreeBLAS(pBlasInstance);
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
