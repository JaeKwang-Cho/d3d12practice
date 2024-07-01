// SpriteObject.cpp from "megayuchi"

#include "pch.h"
#include "SpriteObject.h"
#include "D3D12Renderer.h"
#include "ConstantBufferPool.h"
#include "SingleDescriptorAllocator.h"
#include "DescriptorPool.h"
#include "D3D12ResourceManager.h"
#include "D3DUtil.h"

Microsoft::WRL::ComPtr<ID3D12RootSignature> SpriteObject::m_pRootSignature = nullptr;
//Microsoft::WRL::ComPtr<ID3D12PipelineState> SpriteObject::m_pPipelineState = nullptr;

Microsoft::WRL::ComPtr<ID3D12Resource> SpriteObject::m_pVertexBuffer = nullptr;
D3D12_VERTEX_BUFFER_VIEW SpriteObject::m_VertexBufferView = {};

Microsoft::WRL::ComPtr<ID3D12Resource> SpriteObject::m_pIndexBuffer = nullptr;
D3D12_INDEX_BUFFER_VIEW SpriteObject::m_IndexBufferView = {};

DWORD SpriteObject::m_dwInitRefCount = 0;

bool SpriteObject::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;

	bool bResult = InitCommonResources();
	// 일단은 인스턴스마다 PSO를 가지도록 한다.
	bResult = InitPipelineState();
	return bResult;
}

bool SpriteObject::Initialize(D3D12Renderer* _pRenderer, const WCHAR* _wchTexFileName, const RECT* _pRect)
{
	m_pRenderer = _pRenderer;

	bool bResult = InitCommonResources();
	// 일단은 인스턴스마다 PSO를 가지도록 한다.
	bResult = InitPipelineState();
	if (bResult) {
		UINT texWidth = 1;
		UINT texHeight = 1;
		m_pTexHandle = (TEXTURE_HANDLE*)m_pRenderer->CreateTextureFromFile(_wchTexFileName);

		if (m_pTexHandle) {
			D3D12_RESOURCE_DESC desc = m_pTexHandle->pTexResource->GetDesc();
			texWidth = (UINT)desc.Width;
			texHeight = (UINT)desc.Height;
		}
		if (_pRect) {
			m_Rect = *_pRect;
			m_Scale.x = (float)(m_Rect.right - m_Rect.left) / (float)texWidth;
			m_Scale.y = (float)(m_Rect.bottom - m_Rect.top) / (float)texHeight;
		}
		else {
			if (m_pTexHandle) 
			{
				D3D12_RESOURCE_DESC desc = m_pTexHandle->pTexResource->GetDesc();
				m_Rect.left = 0;
				m_Rect.top = 0;
				m_Rect.right = (LONG)desc.Width;
				m_Rect.bottom = (LONG)desc.Height;
			}
		}
	}

	return bResult;
}

void SpriteObject::DrawWithTex(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMFLOAT2* _pPos, const XMFLOAT2* _pScale, const RECT* _pRect, float _z, TEXTURE_HANDLE* _pTexHandle)
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	// Renderer가 관리하는 Pool
	ConstantBufferPool* pConstantBufferPool = m_pRenderer->INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE::SPRITE);
	DescriptorPool* pDescriptorPool = m_pRenderer->INL_DescriptorPool();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pPoolDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();

	// 텍스쳐 정보를 받는다.
	UINT texWidth = 0;
	UINT texHeight = 0;
		
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	if (_pTexHandle) {
		D3D12_RESOURCE_DESC desc = _pTexHandle->pTexResource->GetDesc();
		texWidth = (UINT)desc.Width;
		texHeight = (UINT)desc.Height;
		srv = _pTexHandle->srv;
	}

	RECT rect;
	if (!_pRect) {
		rect.left = 0;
		rect.top = 0;
		rect.right = texWidth;
		rect.bottom = texHeight;
		_pRect = &rect;
	}

	// pool에서 받은 Descriptor를 이용해서 독립적으로 그린다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};

	if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, DESCRIPTOR_COUNT_FOR_DRAW)) {
		__debugbreak();
	}

	CB_CONTAINER* pCB = pConstantBufferPool->Alloc();
	if (!pCB) {
		__debugbreak();
	}

	CONSTANT_BUFFER_SPRITE* pCBSprite = (CONSTANT_BUFFER_SPRITE*)pCB->pSystemMemAddr;
	// CB 값을 업데이트 한다.
	pCBSprite->screenResol.x = (float)m_pRenderer->INL_GetScreenWidth();
	pCBSprite->screenResol.y = (float)m_pRenderer->INL_GetScreenHeight();
	pCBSprite->pos = *_pPos;
	pCBSprite->scale = *_pScale;
	pCBSprite->texSize.x = (float)texWidth;
	pCBSprite->texSize.y = (float)texHeight;
	pCBSprite->texSamplePos.x = (float)_pRect->left;
	pCBSprite->texSamplePos.y = (float)_pRect->top;
	pCBSprite->TexSampleSize.x = (float)(_pRect->right - _pRect->left);
	pCBSprite->TexSampleSize.y = (float)(_pRect->bottom - _pRect->top);
	pCBSprite->z = _z;
	pCBSprite->alpha = 1.f;

	// 루트 시그니처를 세팅한다.
	_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	// 데이터가 올라간 힙도 세팅한다.
	_pCommandList->SetDescriptorHeaps(1, pPoolDescriptorHeap.GetAddressOf());

	// CBV와 SRV를 pooling 한 Descriptor에 복사하고
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuDescriptorTable, (UINT)SPRITE_DESCRIPTOR_INDEX::CBV, srvDescriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, cbvDest, pCB->cbvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srv.ptr) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuDescriptorTable, (UINT)SPRITE_DESCRIPTOR_INDEX::TEX, srvDescriptorSize);
		pD3DDevice->CopyDescriptorsSimple(1, srvDest, srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// 테이블 번호에 맞게 bind 한다.
	_pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	// pso를 세팅하고
	_pCommandList->SetPipelineState(m_pPipelineState.Get());
	// quad를 그린다.
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	_pCommandList->IASetIndexBuffer(&m_IndexBufferView);
	_pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void SpriteObject::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMFLOAT2* _pPos, const XMFLOAT2* _pScale, float _z)
{
	XMFLOAT2 scale = { m_Scale.x * _pScale->x,  m_Scale.y * _pScale->y };
	DrawWithTex(_pCommandList, _pPos, &scale, &m_Rect, _z, m_pTexHandle);
}

bool SpriteObject::InitCommonResources()
{
	if (m_dwInitRefCount) {
		goto RETURN;
	}

	InitRootSignature();
	InitMesh();

RETURN:
	m_dwInitRefCount++;
	return true;
}

void SpriteObject::CleanUpSharedResources()
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
		m_pVertexBuffer = nullptr;
		m_pIndexBuffer = nullptr;
	}
}

bool SpriteObject::InitRootSignature()
{
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	Microsoft::WRL::ComPtr<ID3DBlob> pSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pError = nullptr;

	// table로 root signature를 생성한다.
	CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0 : CBV
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // s0 : SRV (texture)

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

	// default sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// root signature를 만든다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()))) {
		__debugbreak();
	}

	if (FAILED(pD3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf())))) {
		__debugbreak();
	}

	return true;
}

bool SpriteObject::InitPipelineState()
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState = nullptr;

	std::string psoKey = g_PSOKeys[(UINT)PSO_KEYS_INDEX::SPRITE_FILL];
	pPipelineState = m_pRenderer->GetPSO(psoKey);
	if (pPipelineState != nullptr) {
		m_pPipelineState = pPipelineState;
		goto RETURN;
	}
	else {
		Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

		Microsoft::WRL::ComPtr<ID3DBlob> pVertexShader = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pPixelShader = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		// 스프라이트 전용 쉐이더를 컴파일 한다.
		if (FAILED(D3DCompileFromFile(L"./Shaders/Sprite.hlsl", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, pVertexShader.GetAddressOf(), pErrorBlob.GetAddressOf())))
		{
			if (pErrorBlob != nullptr) {
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			}
			__debugbreak();
		}

		if (FAILED(D3DCompileFromFile(L"./Shaders/Sprite.hlsl", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, pPixelShader.GetAddressOf(), pErrorBlob.GetAddressOf())))
		{
			if (pErrorBlob != nullptr) {
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
				pErrorBlob->Release();
			}
			__debugbreak();
		}

		// input layout를 세팅한다.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) + sizeof(XMFLOAT4),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		// PSO 생성한다.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf())))) {
			__debugbreak();
		}


		m_pRenderer->CachePSO(psoKey, m_pPipelineState);
	}
RETURN:

	return true;
}

bool SpriteObject::InitMesh()
{
	bool bResult = false;
	Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDeivce = m_pRenderer->INL_GetD3DDevice();

	UINT srvDescriptorSize = m_pRenderer->INL_GetSrvDescriptorSize();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();

	// Quad로 표현한다.
	BasicVertex Vertices[] =
	{
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
	};

	WORD Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	const UINT VertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), (DWORD)_countof(Vertices), &m_VertexBufferView, &m_pVertexBuffer, Vertices)))
	{
		__debugbreak();
		goto RETURN;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)_countof(Indices), &m_IndexBufferView, &m_pIndexBuffer, Indices)))
	{
		__debugbreak();
		goto RETURN;
	}
	bResult = true;

RETURN:
	return bResult;
}

void SpriteObject::CleanUp()
{
	if (m_pTexHandle) {
		m_pRenderer->DeleteTexture(m_pTexHandle);
		m_pTexHandle = nullptr;
	}
	CleanUpSharedResources();
}

SpriteObject::SpriteObject()
	:m_pTexHandle(nullptr), m_pRenderer(nullptr),
	m_Rect{}, m_Scale{1.f, 1.f}, 
	m_dwTriGroupCount(0), m_dwMaxTriGroupCount(),
	m_pPipelineState(nullptr)
{
}

SpriteObject::~SpriteObject()
{
	CleanUp();
}
