// D3D12Renderer.cpp from "megayuchi"

#include "pch.h"
#include "D3D12Renderer.h"
#include <dxgidebug.h>
#include "D3DUtil.h"
#include "BasicMeshObject.h"
#include "D3D12ResourceManager.h"
#include "ConstantBufferPool.h"
#include "DescriptorPool.h"
#include "SingleDescriptorAllocator.h"

bool D3D12Renderer::Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV)
{
	bool bResult = false;
	HRESULT hr = S_OK;

	// window ���� �ް�
	m_hWnd = _hWnd;

	// debug layer�� �Ѵµ� ����ϴ� interface
	Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController = nullptr;
	// DXGI ��ü�� �����ϴ� interface
	Microsoft::WRL::ComPtr<IDXGIFactory7> pFactory = nullptr;
	// display subsystem�� ������ �˾Ƴ��� interface
	Microsoft::WRL::ComPtr<IDXGIAdapter4> pAdaptor = nullptr;

	DXGI_ADAPTER_DESC3 AdaptorDesc = {};

	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;

	// #1 GPU ����� ���̾� ����
	if (_bEnableDebugLayer) {
		// ������ ��û ������ ������� interface�� ��Ⱑ ������µ�,
		// �Ʒ� �����Լ��� ��û �������ٰ� �Ѵ�.
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
		if (SUCCEEDED(hr)) {
			pDebugController->EnableDebugLayer();
		}

		// #2 GBV(GPU based Validation) ����
		// �Ʒ��� DXGI�� ���ؼ� ����� �÷��׸� �������ִ� ���̴�.
		dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		// D3D12�� CPU Ÿ�Ӷ��ΰ�, GPU Ÿ�Ӷ����� �ٸ���.
		// (driver���� ���ư��� ��, shader���� ���ư��� ���� �ٸ���.)

		// ��·�� ���⼭ �ϴ°�, GPU���� ���� ������ ���ܼ� ������ ����شٴ� ���̴�.
		if (_bEnableGBV) {
			// debug6��� query interface�� �����´�.
			Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController6 = nullptr;
			if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(pDebugController6.GetAddressOf()))) {
				// �� interface�� GBV�� ���ش�.
				pDebugController6->SetEnableGPUBasedValidation(TRUE);
				pDebugController6->SetEnableAutoName(TRUE);
			}
		}
	}

	// #3 DXGIFactory ����
	// ���� DirectX�� �ΰ�(d3d + dxgi)�� �ϳ��� �־��µ�, �װ� 2���� �и��ߴ�.

	// dxgi�� ����ϴ� ������ double-buffering�� �ϱ� �����̴�.
	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory));

	// GPU�� ������ �ִ� ��ɵ��� ������ ��Ÿ���� ��
	// (�������� GPU�� DirectX ǥ�ؿ� ���缭 ����� �������� �ʾҴ�. �׷��� ��� ����� �� Ȯ���ؾ��ߴ�.)
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1, // ������� ����Ʈ���̽��� �����Ѵ�.
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0, // �⺻ ����� ����ε� ����ϴ�.
	};

	DWORD FeatureLevelNum = _countof(featureLevels);

	// #4 GPU feature level�� �´� D3DDevice ����
	for (DWORD flIndex = 0; flIndex < FeatureLevelNum; flIndex++) {
		UINT adaptorIndex = 0;
		// DXGI�� ���� ����߿� �׷��� ī�带 �����ϴ� ��ɵ� �ִ�.
		// DXGIFactory���� ��͸� ���ͼ�
		IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(pAdaptor.GetAddressOf());
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adaptorIndex, pTempAdaptor)) {
			pAdaptor->GetDesc3(&AdaptorDesc);
			// �׷��� ī����� Ȯ���غ��鼭 ���� ������ Ȯ���ϴ� ���̴�.
			// GPU���ٰ� D3DDevice�� �����غ��� �����ϸ� �ش� feature level�� ������ �ִ� ���̴�.
			if (SUCCEEDED(D3D12CreateDevice(pAdaptor.Get(), featureLevels[flIndex], IID_PPV_ARGS(&m_pD3DDevice)))) {
				goto EXIT;
			}
			// ����ȯ�� �ؼ� �׷���.. ����Ʈ �����ͷ� ������ �ȵȴ�.
			(*pTempAdaptor)->Release();
			(*pTempAdaptor) = nullptr;
			adaptorIndex++;
		}
	}
EXIT:
	// d3ddevice�� �����ϰ�
	if (!m_pD3DDevice) {
		__debugbreak();
		goto RETURN;
	}
	m_pD3DDevice->SetName(L"device");
	m_AdaptorDesc = AdaptorDesc;

	// ���� : goto �Ʒ��� �̷��� �ٵ� ���� ����� ������ goto ������, �бⰡ �ɶ��� �ٵ� �־�� ������ �ʱ�ȭ �� �� �ִ�. -> "��Ʈ�� ����"

	// d3d debug �߰� ������ �� �� �ִ�.
	if (pDebugController) {
		// D3DUtil.h
		SetDebugLayerInfo(m_pD3DDevice.Get());
	}

	// #5 �� ������ ���� �ۼ��� Command List�� �ö� Command Queue�� ����
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
		if (FAILED(hr)) {
			__debugbreak();
			goto RETURN;
		}
	}
	m_pCommandQueue->SetName(L"Command Queue");
	// #6 RTV�� Descriptor Heap�� �����Ѵ�.
	CreateDescriptorHeapForRTV();

	// #7 swap chain�� �׿� �ʿ��� ID3DResource�� �����.

	// swapchain �Ӽ��� �����ϰ�
	{
		RECT rect;
		GetClientRect(_hWnd, &rect);
		UINT uiWndWidth = rect.right - rect.left;
		UINT uiWndHeight = rect.bottom - rect.top;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = uiWndWidth;
		swapChainDesc.Height = uiWndHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // Ƽ� ������ ���� �ʴ´�.
		
		m_dwSwapChainFlags = swapChainDesc.Flags;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain1 = nullptr;
		// DXGI�� ������ �ڵ��� ������ swapchain�� �����.
		if (FAILED(pFactory->CreateSwapChainForHwnd(
			m_pCommandQueue.Get(), _hWnd, &swapChainDesc, &fsSwapChainDesc,
			nullptr, pSwapChain1.GetAddressOf()
		))) 
		{
			__debugbreak();
		}
		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// Window�� ���缭 Viewport�� Scissor Rect�� �����Ѵ�.
		m_Viewport.Width = static_cast<float>(uiWndWidth);
		m_Viewport.Height = static_cast<float>(uiWndHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;

		m_ScissorRect.left = 0;
		m_ScissorRect.right = uiWndWidth;
		m_ScissorRect.top = 0;
		m_ScissorRect.bottom = uiWndHeight;
		// �ɹ��� ä���.
		m_dwWidth = static_cast<DWORD>(uiWndWidth);
		m_dwHeight = static_cast<DWORD>(uiWndHeight);
	}

	// #8 ������ Frame�� ���� Frame Resource�� �����.
	{

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
			// Descriptor Heap�� Render Target ������ �ϴ� SwapChain�� ����Ʈ ���۸�
			// ID3D12Resource���� �����ͼ�
			m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
			// �׸��� �װ��� ����Ű�� view(������ ������)�� ���������� ����Ÿ������ �� �� �ְ� �Ѵ�.
			m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
			// RTV Heap handle�� �������� �Ű� �� ���۵� �Ȱ��� �����Ѵ�.
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			m_pRenderTargets[i]->SetName(L"Render Target Resource");
		}
		m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// #9 depth-stencil ���� Heap�� ������ resource view�� �����.
	CreateDescriptorHeapForDSV();
	CreateDepthStencil(m_dwWidth, m_dwHeight);

	// #10 Command List�� �����.
	CreateCommandList();

	// #11 fence�� �����Ѵ�.
	// synchronization objects�� �ʿ��� ������, d3d12�� GPU���� ���ҽ��� ����ϱ� ����
	// �װ��� ������ ���� �� �ִ�. �׷��� �̷��� fence�� ���༭ ���ֱ� ���� Ȯ�� ���ش�.
	// d3d12�� ���� ��ձ�(asynchronous) api��.
	CreateFence();

	// Resource Manager
	m_pResourceManager = new D3D12ResourceManager;
	m_pResourceManager->Initialize(m_pD3DDevice);

	// Command List�� pool�� ���� ������ش�.
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		// Constant Buffer Pool
		m_ppConstantBufferPool[i] = new ConstantBufferPool;
		m_ppConstantBufferPool[i]->Initialize(m_pD3DDevice, AlignConstantBufferSize((UINT)sizeof(CONSTANT_BUFFER_DEFAULT)), MAX_DRAW_COUNT_PER_FRAME);

		// Descriptor Pool
		m_ppDescriptorPool[i] = new DescriptorPool;
		m_ppDescriptorPool[i]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME * BasicMeshObject::DESCRIPTOR_COUNT_FOR_DRAW); // draw call �� ���� Descriptor �ϳ��� �Ѿ��.

	}
	// SingleDescriptorAllocator
	m_pSingleDescriptorAllocator = new SingleDescriptorAllocator;
	m_pSingleDescriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPRTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	InitCamera();

	bResult = true;
RETURN:
	/*
	if (pDebugController)
	{
		pDebugController->Release();
		pDebugController = nullptr;
	}
	if (pAdaptor)
	{
		pAdaptor->Release();
		pAdaptor = nullptr;
	}
	if (pFactory)
	{
		pFactory->Release();
		pFactory = nullptr;
	}
	*/
	return bResult;
}

void D3D12Renderer::BeginRender()
{
	// ȭ�� Ŭ���� �� �̹� ������ �������� ���� �ڷᱸ�� �ʱ�ȭ

	// ���� Rendering�� �� Command Allocator�� List�� ���ؼ� �ʱ�ȭ�� �����Ѵ�.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator = m_ppCommandAllocator[m_dwCurContextIndex];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList = m_ppCommandList[m_dwCurContextIndex];

	if (FAILED(pCommandAllocator->Reset())) {
		__debugbreak();
	}
	// ������ PSO�� ���� ������ nullptr���� �ʱ�ȭ �Ѵ�.
	if (FAILED(pCommandList->Reset(pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
	}

	// ���� ����� �ε����� �´� RTV�� ���´�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	// �׸��� RT Resource ���� �׸� �� �ְ�, PRESENT���� RENDER_TARGET���� �ٲ��ش�.
	// (�̰� ���� ���� ����� API�� D3D12�� ���� Resource�� ��ȣ�ϴ� ����̴�.)
	D3D12_RESOURCE_BARRIER trans_PRESENT_RT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &trans_PRESENT_RT);

	// DSV�� ���´�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// command�� ����Ѵ�.
	// RTV handle�� �ʱ�ȭ �� ���� �־��ش�.
	pCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
	// DSV�� ������ �ʱ�ȭ ���ش�.
	pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// viewport, scissor rect, render target�� ���������
	// �� ���� ������ �׸� �� �ִ�.
	pCommandList->RSSetViewports(1, &m_Viewport);
	pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	// ���� z���۸� �Բ� �־��ش�.
	pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void D3D12Renderer::EndRender()
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList = m_ppCommandList[m_dwCurContextIndex];

	// �׸� ���� �� �׷����� ���� Render target�� ���¸� ResourceBarrier ���¸� PRESENT�� �ٲ۴�.
	D3D12_RESOURCE_BARRIER trans_RT_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &trans_RT_PRESENT);

	// �������� close�� �ɰ�
	pCommandList->Close();

	// queue�� �Ѱ��ش�. (��������� ������ �� �����Ӹ��� �ϴ� ���̴�.)
	ID3D12CommandList* ppCommandLists[] = { pCommandList.Get()};
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

}

void D3D12Renderer::Present()
{
	// fence ������ ���� CommandList�� ���ؼ� �Ѵ�.
	DoFence();

	// back Buffer ȭ���� Primary Buffer�� ����
	UINT SyncInterval = 1; // Vsync on , 0�̸� Vsync�� off �ϴ� ���̴�.

	UINT uiSyncInterval = SyncInterval;
	UINT uiPresentFlags = 0;

	// ������� �ֻ����� GPU Rendering �ֱ���� ���̿��� ����� ȭ���� �������� �����̴�.
	if (!uiSyncInterval) {
		// �̷��� �ؾ� Tearing(ȭ�� ������)�� �����ϰ�
		// Vsync�� ���ش�.
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}
	// �� ���ۿ� ����Ʈ ���۸� �ٲ۴�.
	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		__debugbreak();
	}

	// �� ���۷� �ٲ� ģ���� ���� �����ӿ� �׸� �ε����� �����Ѵ�.
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// ���� ������ �۾��� �ϱ� ����, ���� �������� CommandList�� �ش��ϴ� Fence���� �����ߴ��� Ȯ���Ѵ�.
	DWORD dwNextContextIndex = (m_dwCurContextIndex + 1) & MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_pui64LastFenceValue[dwNextContextIndex]);

	// �� �������� �������� 0���� �ʱ�ȭ �Ѵ�.
	m_ppConstantBufferPool[dwNextContextIndex]->Reset();
	m_ppDescriptorPool[dwNextContextIndex]->Reset();
}

bool D3D12Renderer::UpdateWindowSize(DWORD _dwWidth, DWORD _dwHeight)
{
	// ��ȿ���� ���� ũ�⳪
	if (!(_dwWidth * _dwHeight)) {
		return false;
	}
	// ũ�Ⱑ ������ �ʾ�����, �������� �ʴ´�.
	if (_dwWidth == m_dwWidth && _dwHeight == m_dwHeight) {
		return false;
	}

	// ��� CommandList�� ���ؼ� wait�� �Ǵ�.
	DoFence();
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	// ���� ü�� ������ �����س���
	HRESULT hr = m_pSwapChain->GetDesc1(&desc);
	if (FAILED(hr)) {
		__debugbreak();
	}
	// ���� �ִ� RT Resource�� �������ְ�
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		m_pRenderTargets[i]->Release();
		m_pRenderTargets[i] = nullptr;
	}
	// ���ο� ���۸� �����Ѵ�.
	hr = m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, _dwWidth, _dwHeight, DXGI_FORMAT_R8G8B8A8_UNORM, m_dwSwapChainFlags);
	if (FAILED(hr)) {
		__debugbreak();
	}
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// ���ο� ���۸� ����Ű�� Resource View�� �����Ѵ�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// �ɹ��� ������Ʈ ���ش�.
	m_dwWidth = _dwWidth;
	m_dwHeight = _dwHeight;
	m_Viewport.Width = static_cast<float>(_dwWidth);
	m_Viewport.Height = static_cast<float>(_dwHeight);
	m_ScissorRect.left = 0;
	m_ScissorRect.right = _dwWidth;
	m_ScissorRect.top = 0;
	m_ScissorRect.bottom = _dwHeight;

	InitCamera();

	return true;
}

void* D3D12Renderer::CreateBasicMeshObject_Return_New()
{
	BasicMeshObject* pMeshObj = new BasicMeshObject;
	pMeshObj->Initialize(this);

	//pMeshObj->CreateMesh_UploadHeap();
	//pMeshObj->CreateMesh_DefaultHeap();
	//pMeshObj->CreateMesh_WithIndex();
	//pMeshObj->CreateMesh_WithTexture();
	//pMeshObj->CreateMesh_WithCB();
	pMeshObj->CreateMesh();

	return pMeshObj;
}

void D3D12Renderer::DeleteBasicMeshObject(void* _pMeshObjectHandle)
{
	// �̷��� ����ȯ�� �ؾ� ������ �� �����. (�޸� ũ�� + �Ҹ��� ȣ��)
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjectHandle);
	delete pMeshObj;
}

void D3D12Renderer::RenderMeshObject(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld, void* _pTexHandle)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> pCommandList = m_ppCommandList[m_dwCurContextIndex];

	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjectHandle);

	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	if (_pTexHandle) {
		srv = ((TEXTURE_HANDLE*)_pTexHandle)->srv;
	}
	pMeshObj->Draw(pCommandList.Get(), pMatWorld, srv);
}

void* D3D12Renderer::CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b)
{
	TEXTURE_HANDLE* pTexHandle = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	UINT pixSize = sizeof(uint32_t);
	BYTE* pImage = (BYTE*)malloc(_texWidth * _texHeight * pixSize);
	memset(pImage, 0, _texWidth *_texHeight * pixSize);

	BOOL bWhiteStart = TRUE;

	for (UINT y = 0; y < _texHeight; y++) {
		for (UINT x = 0; x < _texWidth; x++) {
			RGBA* pDest = (RGBA*)(pImage + pixSize * (y * _texWidth + x));

			if ((bWhiteStart + x) % 2) {
				pDest->r = _r;
				pDest->g = _g;
				pDest->b = _b;
			}
			else {
				pDest->r = 0;
				pDest->g = 0;
				pDest->b = 0;
			}
			pDest->a = 255;
		}
		bWhiteStart++;
		bWhiteStart %= 2;
	}


	HRESULT hr = m_pResourceManager->CreateTexture(&pTexResource, _texWidth, _texHeight, texFormat, pImage);
	if (SUCCEEDED(hr)) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texFormat;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// SingleDescriptorAllocator���� �� �ڸ��� �޾� Texture Resource�� View�� �Ҵ��Ѵ�.
		if (m_pSingleDescriptorAllocator->AllocDescriptorHandle(&srv)) {
			m_pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &srvDesc, srv);

			pTexHandle = new TEXTURE_HANDLE;
			pTexHandle->pTexResource = pTexResource;
			pTexHandle->srv = srv;
		}
	}
	free(pImage);
	pImage = nullptr;

	return pTexHandle;
}

void* D3D12Renderer::CreateTextureFromFile(const WCHAR* _wchFileName)
{
	TEXTURE_HANDLE* pTexHandle = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RESOURCE_DESC desc = {};
	if (SUCCEEDED(m_pResourceManager->CreateTextureFromFile(&pTexResource, &desc, _wchFileName))) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;

		if (m_pSingleDescriptorAllocator->AllocDescriptorHandle(&srv)) {
			m_pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &srvDesc, srv);

			pTexHandle = new TEXTURE_HANDLE;
			pTexHandle->pTexResource = pTexResource;
			pTexHandle->srv = srv;
		}
	}

	return pTexHandle;
}

void D3D12Renderer::DeleteTexture(void* _pTexHandle)
{
	// ���� �����Ҷ��� �̷��� GPU �۾��� �����⸦ ��ٷ��� �Ѵ�.
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	TEXTURE_HANDLE* pTexHandle = (TEXTURE_HANDLE*)_pTexHandle;
	// ����Ʈ �����͸� ������� ������ release�� ����� �Ѵ�.
	pTexHandle->pTexResource = nullptr;
	m_pSingleDescriptorAllocator->FreeDescriptorHandle(pTexHandle->srv);

	delete pTexHandle;
}

void D3D12Renderer::CreateCommandList()
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> pCommandList = nullptr;

		// command list�� command allocator�� command list�� �������� �ִ�.
		if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pCommandAllocator.GetAddressOf())))) {
			__debugbreak();
		}
		pCommandAllocator->SetName(L"Command Allocator");
		if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(pCommandList.GetAddressOf())))) {
			__debugbreak();
		}
		pCommandList->SetName(L"Command List");
		// �ϴ� ó������ close�� ���ش�.
		pCommandList->Close();

		m_ppCommandAllocator[i] = pCommandAllocator;
		m_ppCommandList[i] = pCommandList;
	}
}

bool D3D12Renderer::CreateDescriptorHeapForRTV()
{
	// Render Target�� Descriptor heap�� �����.
	// Render Target�� ID3D12Resource�� ����Ѵ�.
	// �׷��� RT�� GPU�� ����� ����(���� �޸�)�� �������� �ϴ� ���̴�.
	// (Descriptor Heap�� �����ϴ� ���� Descriptor Table�̴�.)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // render target�� �´� Ÿ���� �������ش�.
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf())))) {
		__debugbreak();
	}
	m_pRTVHeap->SetName(L"Render Target Heap");
	// Render Target view�� offset stride size�� ������ ���´�.
	m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	return true;
}

bool D3D12Renderer::CreateDescriptorHeapForDSV()
{
	// Depth-Stencil�� DescriptorHeap�� �����.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1; // �ϴ� �⺻���� �ϳ��� �����.
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_pDSVHeap.GetAddressOf())))) {
		__debugbreak();
	}
	m_dsvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	return true;
}

bool D3D12Renderer::CreateDepthStencil(UINT _width, UINT _height)
{
	// Depth-Stencil ���� ������ Texture View�� �����Ѵ�.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
	dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsDesc.Flags = D3D12_DSV_FLAG_NONE;

	// �ʱ�ȭ�� ���� �����Ѵ�.
	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC depthDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		_width,
		_height,
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	// resource�� �����
	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&HEAP_PROPS_DEFAULT,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(m_pDepthStencil.GetAddressOf())
	))) {
		__debugbreak();
	}

	// heap�� view�� �ø���.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pD3DDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &dsDesc, dsvHandle);

	return true;
}

void D3D12Renderer::InitCamera()
{
	// ���� �⺻���� ī�޶� �������� �Ѵ�.
	XMVECTOR eyePos = XMVectorSet(0.f, 0.f, -1.f, 1.f);
	XMVECTOR eyeDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	XMVECTOR upDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	// View (DirectX�� �⺻������ �޼� ��ǥ��)
	m_matView = XMMatrixLookAtLH(eyePos, eyeDir, upDir);

	// FOV
	float fovY = XM_PIDIV4;

	// Projection
	float fAspectRatio = (float)m_dwWidth / (float)m_dwHeight;
	float fNear = 0.1f;
	float fFar = 1000.f;

	m_matProj = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNear, fFar);
}

void D3D12Renderer::CreateFence()
{
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}
	m_pFence->SetName(L"Fence");
	m_ui64FenceValue = 0;
	// �ش� fence���� GPU�� �۾��� ���ƴ����� window event�� ó���Ѵ�.
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void D3D12Renderer::CleanupFence()
{
	if (m_hFenceEvent)
	{
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
}

UINT64 D3D12Renderer::DoFence()
{
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	// �̷��� Command list ���� �����ؾ��� Fence ���� ���ε��� ������ش�.
	m_pui64LastFenceValue[m_dwCurContextIndex] = m_ui64FenceValue;
	return m_ui64FenceValue;
}

void D3D12Renderer::WaitForFenceValue(UINT64 _expectedFenceValue)
{
	// fence�� ��ٸ���.
	if (m_pFence->GetCompletedValue() < _expectedFenceValue) {
		m_pFence->SetEventOnCompletion(_expectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3D12Renderer::CleanUpRenderer()
{
	// Ȥ�� �������� GPU �۾��� ������ �Ѵ�.
	DoFence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		if (m_ppConstantBufferPool[i]) {
			delete m_ppConstantBufferPool[i];
			m_ppConstantBufferPool[i] = nullptr;
		}

		if (m_ppDescriptorPool[i]) {
			delete m_ppDescriptorPool[i];
			m_ppDescriptorPool[i] = nullptr;
		}
	}
	if (m_pResourceManager) {
		delete m_pResourceManager;
		m_pResourceManager = nullptr;
	}

	if (m_pSingleDescriptorAllocator) {
		delete m_pSingleDescriptorAllocator;
		m_pSingleDescriptorAllocator = nullptr;
	}


	CleanupFence();
}

D3D12Renderer::D3D12Renderer()
	: m_hWnd(nullptr), m_pD3DDevice(nullptr), m_pCommandQueue(nullptr), m_ppCommandAllocator{},
	m_pResourceManager(nullptr), m_ppConstantBufferPool{}, m_ppDescriptorPool{}, m_pSingleDescriptorAllocator(nullptr),
	m_ppCommandList{}, m_ui64FenceValue(0), m_pui64LastFenceValue{},
	m_FeatureLevel(D3D_FEATURE_LEVEL_11_0),
	m_AdaptorDesc{}, m_pSwapChain(nullptr), m_pRenderTargets{}, m_pDepthStencil(nullptr),
	m_pRTVHeap(nullptr), m_pDSVHeap(nullptr), m_pSRVHeap(nullptr),
	m_rtvDescriptorSize(0), m_srvDescriptorSize(0), m_dsvDescriptorSize(0),
	m_dwSwapChainFlags(0), m_uiRenderTargetIndex(0),
	m_hFenceEvent(nullptr), m_pFence(nullptr), m_dwCurContextIndex(0),
	m_Viewport{}, m_ScissorRect{},m_dwWidth(0),m_dwHeight(0),
	m_matView{}, m_matProj{}
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanUpRenderer();
}

