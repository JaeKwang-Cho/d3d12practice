#include "pch.h"
#include "TextureRenderMesh.h"

#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "D3DUtil.h"
#include "ConstantBufferPool.h"
#include "DescriptorPool.h"

Microsoft::WRL::ComPtr<ID3D12RootSignature> TextureRenderMesh::m_pRootSignature = nullptr;
DWORD TextureRenderMesh::m_dwInitRefCount = 0;

bool TextureRenderMesh::Initialize(D3D12Renderer* _pRenderer, D3D_PRIMITIVE_TOPOLOGY _primitiveTopoloy)
{
	m_pRenderer = _pRenderer;
	m_PrimitiveTopoloy = _primitiveTopoloy;

	bool bResult = InitCommonResources();

	// 일단은 인스턴스마다 PSO를 갖도록 한다.
	bResult = InitPipelineState();
	return bResult;
}

void TextureRenderMesh::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld)
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	// Renderer가 관리하는 Pool
	ConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool(E_CONSTANT_BUFFER_TYPE::DEFAULT);
	DescriptorPool* pDescriptorPool = m_pRenderer->INL_DescriptorPool();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pPoolDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	// 일단은 subRenderGeoCount + 1만큼 pool에서 할당 받는다. CB 한개 + SubRenderGeoCount 만큼 가진 Texture 개수
	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, m_subRenderGeoCount + 1)) {
		__debugbreak();
	}

	CB_CONTAINER* pCB = pConstantBufferPool->Alloc();
	if (!pCB) {
		__debugbreak();
	}

	CONSTANT_BUFFER_OBJECT* pConstantBufferDefault = reinterpret_cast<CONSTANT_BUFFER_OBJECT*>(pCB->pSystemMemAddr);
	// CB 값을 업데이트 하고
	XMMATRIX viewMat;
	XMMATRIX projMat;
	m_pRenderer->GetViewProjMatrix(&viewMat, &projMat);

	pConstantBufferDefault->matWorld = XMMatrixTranspose(*_pMatWorld);
	XMVECTOR detWorld = XMMatrixDeterminant(*_pMatWorld);
	pConstantBufferDefault->invWorldTranspose = XMMatrixInverse(&detWorld, *_pMatWorld);

	// 초기화 할 때 정한 Texture와 업데이트한 CB를 넘길, Descriptor Table을 구성한다.

	// Object 마다 넘어가는 CB는 CopyDescriptorSimple을 이용해서, 현재 Constant Buffer View를 현재 할당된 Descriptor Heap 위치에 있는 Descriptor에 복사한다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(cpuDescriptorTable, static_cast<INT>(E_TEX_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ::CBV), srvDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, dest, pCB->cbvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // CPU 코드에서는 CPU Handle에 write만 가능하다.

	// pool에서 allocation 받았기 때문에 선형인 Heap위에 있기에 
	dest.Offset(1, srvDescriptorSize); // 이렇게 같은 핸들을 offset을 이용해 다음 자리를 구하면 된다.

	for (UINT i = 0; i < m_subRenderGeoCount; i++)
	{
		SubRenderGeometry* pSubRenderGeo = m_subRenderGeometries[i];
		TEXTURE_HANDLE* pTexHandle = pSubRenderGeo->pTexHandle;
		if (pTexHandle)
		{
			pD3DDevice->CopyDescriptorsSimple(1, dest, pTexHandle->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			pD3DDevice->CopyDescriptorsSimple(1, dest, DEFAULT_WHITE_TEXTURE->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		dest.Offset(1, srvDescriptorSize);
	}

	// 루트 시그니처를 설정하고
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// pool에 있는 Heap으로 설정한다.
	_pCommandList->SetDescriptorHeaps(1, pPoolDescriptorHeap.GetAddressOf());

	// 일단 테이블 번호에 맞춰서 0번으로 CBV를 넘겨준다. 지금은 1개만 넘겨준다.
	_pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	// FrameCB도 넘겨준다.
	_pCommandList->SetGraphicsRootConstantBufferView(2, m_pRenderer->INL_GetFrameCBResource()->GetGPUVirtualAddress());

	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	_pCommandList->IASetPrimitiveTopology(m_PrimitiveTopoloy);

	for (UINT i = 0; i < m_subRenderGeoCount; i++)
	{
		SubRenderGeometry* pSubRenderGeo = m_subRenderGeometries[i];
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTexture(gpuDescriptorTable, (UINT)E_TEX_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ::TEX, srvDescriptorSize);
		if (pSubRenderGeo) {
			_pCommandList->IASetVertexBuffers(0, 1, &pSubRenderGeo->m_VertexBufferView);
			_pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTexture);

			_pCommandList->IASetIndexBuffer(&pSubRenderGeo->m_IndexBufferView);
			_pCommandList->DrawIndexedInstanced(3, 1, pSubRenderGeo->startIndexLocation, pSubRenderGeo->baseVertexLocation, 0);
		}
		gpuDescriptorTableForTexture.Offset(1, srvDescriptorSize);
	}
}

void TextureRenderMesh::DrawOutline(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld)
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	// Renderer가 관리하는 Pool
	ConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool(E_CONSTANT_BUFFER_TYPE::DEFAULT);
	DescriptorPool* pDescriptorPool = m_pRenderer->INL_DescriptorPool();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pPoolDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	// 일단은 subRenderGeoCount + 1만큼 pool에서 할당 받는다. CB 한개 + SubRenderGeoCount 만큼 가진 Texture 개수
	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, m_subRenderGeoCount + 1)) {
		__debugbreak();
	}

	CB_CONTAINER* pCB = pConstantBufferPool->Alloc();
	if (!pCB) {
		__debugbreak();
	}

	CONSTANT_BUFFER_OBJECT* pConstantBufferDefault = reinterpret_cast<CONSTANT_BUFFER_OBJECT*>(pCB->pSystemMemAddr);
	// CB 값을 업데이트 하고
	XMMATRIX viewMat;
	XMMATRIX projMat;
	m_pRenderer->GetViewProjMatrix(&viewMat, &projMat);

	pConstantBufferDefault->matWorld = XMMatrixTranspose(*_pMatWorld);
	XMVECTOR detWorld = XMMatrixDeterminant(*_pMatWorld);
	pConstantBufferDefault->invWorldTranspose = XMMatrixInverse(&detWorld, *_pMatWorld);

	// 초기화 할 때 정한 Texture와 업데이트한 CB를 넘길, Descriptor Table을 구성한다.

	// Object 마다 넘어가는 CB는 CopyDescriptorSimple을 이용해서, 현재 Constant Buffer View를 현재 할당된 Descriptor Heap 위치에 있는 Descriptor에 복사한다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dest(cpuDescriptorTable, static_cast<INT>(E_TEX_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ::CBV), srvDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, dest, pCB->cbvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // CPU 코드에서는 CPU Handle에 write만 가능하다.

	// pool에서 allocation 받았기 때문에 선형인 Heap위에 있기에 
	dest.Offset(1, srvDescriptorSize); // 이렇게 같은 핸들을 offset을 이용해 다음 자리를 구하면 된다.

	for (UINT i = 0; i < m_subRenderGeoCount; i++)
	{
		SubRenderGeometry* pSubRenderGeo = m_subRenderGeometries[i];
		TEXTURE_HANDLE* pTexHandle = pSubRenderGeo->pTexHandle;
		if (pTexHandle)
		{
			pD3DDevice->CopyDescriptorsSimple(1, dest, pTexHandle->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		else
		{
			pD3DDevice->CopyDescriptorsSimple(1, dest, DEFAULT_WHITE_TEXTURE->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		dest.Offset(1, srvDescriptorSize);
	}

	// 루트 시그니처를 설정하고
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// pool에 있는 Heap으로 설정한다.
	_pCommandList->SetDescriptorHeaps(1, pPoolDescriptorHeap.GetAddressOf());

	// 일단 테이블 번호에 맞춰서 0번으로 CBV를 넘겨준다. 지금은 1개만 넘겨준다.
	_pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	// FrameCB도 넘겨준다.
	_pCommandList->SetGraphicsRootConstantBufferView(2, m_pRenderer->INL_GetFrameCBResource()->GetGPUVirtualAddress());

	// outline을 그리는 PSO로 세팅해준다.
	_pCommandList->SetPipelineState(m_pPipelineState_Outline.Get());
	// Triangle list with adjacency로 그린다.
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);

	for (UINT i = 0; i < m_subRenderGeoCount; i++)
	{
		SubRenderGeometry* pSubRenderGeo = m_subRenderGeometries[i];
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTexture(gpuDescriptorTable, (UINT)E_TEX_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ::TEX, srvDescriptorSize);
		if (pSubRenderGeo) {
			_pCommandList->IASetVertexBuffers(0, 1, &pSubRenderGeo->m_VertexBufferView);
			_pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTexture);
			// index 버퍼도 잘 설정해준다.
			_pCommandList->IASetIndexBuffer(&pSubRenderGeo->m_AdjIndexBufferView);
			//_pCommandList->DrawIndexedInstanced(pSubRenderGeo->adjIndexCount, 1, pSubRenderGeo->startIndexLocation, pSubRenderGeo->baseVertexLocation, 0);
			_pCommandList->DrawIndexedInstanced(6, 1, pSubRenderGeo->startIndexLocation, pSubRenderGeo->baseVertexLocation, 0);
		}
		gpuDescriptorTableForTexture.Offset(1, srvDescriptorSize);
	}
}

void TextureRenderMesh::CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount)
{
	if (_meshDataCount > MAX_SUB_RENDER_GEO_COUNT) {
		__debugbreak();
		return;
	}

	m_subRenderGeoCount = _meshDataCount;

	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	for (UINT i = 0; i < m_subRenderGeoCount; i++) {
		m_subRenderGeometries[i] = new SubRenderGeometry;
		TextureMeshData& pCurMeshData = _ppMeshData[i];

		// TextureVertex Buffer 먼저 생성한다.
		if (FAILED(pResourceManager->CreateVertexBuffer(
			sizeof(TextureVertex), pCurMeshData.Vertices.size(),
			&(m_subRenderGeometries[i]->m_VertexBufferView),
			&(m_subRenderGeometries[i]->m_pVertexBuffer),
			(void*)pCurMeshData.Vertices.data()
		)))
		{
			__debugbreak();
		}

		// Index Buffer도 생성한다.
		if (FAILED(pResourceManager->CreateIndexBuffer(
			pCurMeshData.Indices32.size(),
			&(m_subRenderGeometries[i]->m_IndexBufferView),
			&(m_subRenderGeometries[i]->m_pIndexBuffer),
			(void*)(pCurMeshData.GetIndices16().data())
		)))
		{
			__debugbreak();
		}
		m_subRenderGeometries[i]->indexCount = pCurMeshData.Indices32.size();
		m_subRenderGeometries[i]->startIndexLocation = 0;
		m_subRenderGeometries[i]->baseVertexLocation = 0;
	}
}

void TextureRenderMesh::CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount, std::vector<uint32_t>& _adjIndices)
{
	CreateRenderAssets(_ppMeshData, _meshDataCount);

	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	for (UINT i = 0; i < m_subRenderGeoCount; i++)
	{
		// Adj - Index Buffer도 생성한다.
		if (FAILED(pResourceManager->CreateIndexBuffer(
			_adjIndices.size(),
			&(m_subRenderGeometries[i]->m_AdjIndexBufferView),
			&(m_subRenderGeometries[i]->m_pAdjIndexBuffer),
			(void*)(_adjIndices.data()),
			sizeof(uint32_t)
		)))
		{
			__debugbreak();
		}
		m_subRenderGeometries[i]->adjIndexCount = _adjIndices.size();
	}
}

void TextureRenderMesh::BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex)
{
	if (m_subRenderGeoCount <= _subRenderAssetIndex)
	{
		__debugbreak();
	}
	m_subRenderGeometries[_subRenderAssetIndex]->pTexHandle = _pTexHandle;
}

bool TextureRenderMesh::InitCommonResources()
{
	// root signature을 싱글톤으로 사용한다.
	if (m_dwInitRefCount > 0) {
		goto EXIST;
	}

	InitRootSignature();

EXIST:
	m_dwInitRefCount++;
	return true;
}

void TextureRenderMesh::CleanupSharedResources()
{
	if (!m_dwInitRefCount) {
		return;
	}

	DWORD refCount = --m_dwInitRefCount;
	// 아직 참조하는 Object가 있다면 삭제하지 않기
	if (!refCount) {
		// 얘네는 data section에 있는 애들이라 직접 해제를 해줘야 한다.
		m_pRootSignature = nullptr;
		//m_pPipelineState = nullptr;
	}
}

bool TextureRenderMesh::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	// 일단 CB 하나 SRV 하나 넘어가는거로 한다.
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;


	CD3DX12_DESCRIPTOR_RANGE rangePerObj[1] = {};
	rangePerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 :  Object 마다 넘기는 Constant Buffer View

	CD3DX12_DESCRIPTOR_RANGE rangePerSub[1] = {};
	rangePerSub[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 :  일단은 Sub - Object 마다 넘기는 SRV(texture)

	// table 0번과 1번에 저장한다.
	// 그리고 CBV로 2번에 저장한다.
	CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangePerObj), rangePerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangePerSub), rangePerSub, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // b1 : Frame 마다 넘기는 Constant Buffer View


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
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}
	// Root signature를 만든다.
	if (FAILED(pD3DDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf())))) {
		__debugbreak();
	}

	return true;
}

bool TextureRenderMesh::InitPipelineState()
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState = nullptr;
	std::string psoKey = g_PSOKeys[(UINT)E_PSO_KEYS_INDEX::DEFAULT_FILL];
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
		// 쉐이더를 컴파일 할때, 최적화를 안하고 디버깅하기 편하도록 하는 것이다.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlas = 0;
#endif
		HRESULT hr;

		// vertex shader를 컴파일하고
		hr = D3DCompileFromFile(L".\\Shaders\\Default.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, pVertexShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
		// pixel shader도 컴파일 한다.
		hr = D3DCompileFromFile(L".\\Shaders\\Default.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, pPixelShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}

		// input layout을 정의하고
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3)*2,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3)*3,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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

		m_pRenderer->CachePSO(psoKey, m_pPipelineState);
	}

RETURN:
	return InitPipelineState_Outline();
}

bool TextureRenderMesh::InitPipelineState_Outline()
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState = nullptr;
	std::string psoKey = g_PSOKeys[(UINT)E_PSO_KEYS_INDEX::OUTLINE];
	pPipelineState = m_pRenderer->GetPSO(psoKey);
	if (pPipelineState != nullptr) {
		m_pPipelineState = pPipelineState;
		goto RETURN;
	}
	else {
		Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

		Microsoft::WRL::ComPtr<ID3DBlob> pVertexShaderBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pGeoShaderBlob = nullptr;
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
		hr = D3DCompileFromFile(L".\\Shaders\\Outline.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, pVertexShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
		// Geometry shader 컴파일하고
		hr = D3DCompileFromFile(L".\\Shaders\\Outline.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_1", compileFlags, 0, pGeoShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
		// pixel shader도 컴파일 한다.
		hr = D3DCompileFromFile(L".\\Shaders\\Outline.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, pPixelShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}

		// input layout을 정의하고
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3) * 2,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 3,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Pipeline State Object (PSO)를 만든다.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize());
		psoDesc.GS = CD3DX12_SHADER_BYTECODE(pGeoShaderBlob->GetBufferPointer(), pGeoShaderBlob->GetBufferSize());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize());

		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;

		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 생성했던 리소스와 동일한 포맷을 이용한다.

		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleMask = UINT_MAX; // 지 혼자 0으로 초기회 된다.

		if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState_Outline.GetAddressOf())))) {
			__debugbreak();
		}

		m_pRenderer->CachePSO(psoKey, m_pPipelineState_Outline);
	}

RETURN:
	return true;
}

void TextureRenderMesh::CleanUpAssets()
{
	for (UINT i = 0; i < m_subRenderGeoCount; i++) {
		if (m_subRenderGeometries[i]) {
			delete m_subRenderGeometries[i];
			m_subRenderGeometries[i] = nullptr;
		}
	}
	CleanupSharedResources();
}

TextureRenderMesh::TextureRenderMesh()
	:m_pRenderer(nullptr), m_PrimitiveTopoloy(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
	m_subRenderGeometries{ nullptr, },
	m_subRenderGeoCount(0), m_pPipelineState(nullptr)
{
}

TextureRenderMesh::~TextureRenderMesh()
{
	CleanUpAssets();
}
