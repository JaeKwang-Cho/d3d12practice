// D3D12Renderer.cpp from "megayuchi"

#include "pch.h"
#include "D3D12Renderer.h"
#include <dxgidebug.h>
#include "D3DUtil.h"
#include "BasicMeshObject.h"

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
	// #6 Descriptor Heap�� �����Ѵ�.
	CreateDescriptorHeap();

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
	}

	// #9 Command List�� �����.
	CreateCommandList();

	// #10 fence�� �����Ѵ�.
	// synchronization objects�� �ʿ��� ������, d3d12�� GPU���� ���ҽ��� ����ϱ� ����
	// �װ��� ������ ���� �� �ִ�. �׷��� �̷��� fence�� ���༭ ���ֱ� ���� Ȯ�� ���ش�.
	// d3d12�� ���� ��ձ�(asynchronous) api��.
	CreateFence();

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

	// d3d12 ������ ȭ�� Ŭ���� �ϴ� �͵� command list�� ����� �ۼ��ؾ� �Ѵ�.
	// �ϴ� allocator�� list�� �ʱ�ȭ ���ְ�
	if (FAILED(m_pCommandAllocator->Reset())) {
		__debugbreak();
	}
	// ������ PSO�� ���� ������ nullptr���� �ʱ�ȭ �Ѵ�.
	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
	}

	// ���� ����� �ε����� �´� RTV�� ���´�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	// �׸��� RT Resource ���� �׸� �� �ְ�, PRESENT���� RENDER_TARGET���� �ٲ��ش�.
	// (�̰� ���� ���� ����� API�� D3D12�� ���� Resource�� ��ȣ�ϴ� ����̴�.)
	D3D12_RESOURCE_BARRIER trans_PRESENT_RT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &trans_PRESENT_RT);

	// command�� ����Ѵ�.
	// RTV handle�� �ʱ�ȭ �� ���� �־��ش�. (z ���۴� �����Ǿ���.)
	m_pCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

	// viewport, scissor rect, render target�� ���������
	// �� ���� ������ �׸� �� �ִ�.
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	// ���絵 z ���۴� �����Ǿ���.
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void D3D12Renderer::EndRender()
{
	// �׸� ���� �� �׷����� ���� Render target�� ���¸� ResourceBarrier ���¸� PRESENT�� �ٲ۴�.
	D3D12_RESOURCE_BARRIER trans_RT_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &trans_RT_PRESENT);

	// �������� close�� �ɰ�
	m_pCommandList->Close();

	// queue�� �Ѱ��ش�. (��������� ������ �� �����Ӹ��� �ϴ� ���̴�.)
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get()};
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

}

void D3D12Renderer::Present()
{
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

	// ���� ������ �۾��� �ϱ� ����
	// ���ҽ� ����ȭ�� ���� queue �۾��� GPU�� ���´���, fence�� Ȯ���Ѵ�.
	DoFence();

	WaitForFenceValue();
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

	return true;
}

void* D3D12Renderer::CreateBasicMeshObject_Return_New()
{
	BasicMeshObject* pMeshObj = new BasicMeshObject;
	pMeshObj->Initialize(this);
	pMeshObj->CreateMesh();

	return pMeshObj;
}

void D3D12Renderer::DeleteBasicMeshObject(void* _pMeshObjectHandle)
{
	// �̷��� ����ȯ�� �ؾ� ������ �� �����. (�޸� ũ�� + �Ҹ��� ȣ��)
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjectHandle);
	delete pMeshObj;
}

void D3D12Renderer::RenderMeshObject(void* _pMeshObjectHandle)
{
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjectHandle);
	pMeshObj->Draw(m_pCommandList.Get());
}

void D3D12Renderer::CreateCommandList()
{
	// command list�� command allocator�� command list�� �������� �ִ�.
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandAllocator->SetName(L"Command Allocator");
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandList->SetName(L"Command List");
	// �ϴ� ó������ close�� ���ش�.
	m_pCommandList->Close();
}

bool D3D12Renderer::CreateDescriptorHeap()
{
	HRESULT hr = S_OK;

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

void D3D12Renderer::CreateFence()
{
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}
	m_pFence->SetName(L"Fence");
	m_ui64enceValue = 0;
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
	m_ui64enceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64enceValue);
	return m_ui64enceValue;
}

void D3D12Renderer::WaitForFenceValue()
{
	const UINT64 ExpectedFenceValue = m_ui64enceValue;
	// fence�� ��ٸ���.
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue) {
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3D12Renderer::CleanUpRenderer()
{
	// Ȥ�� �������� GPU �۾��� ������ �Ѵ�.
	WaitForFenceValue();

	CleanupFence();
}

D3D12Renderer::D3D12Renderer()
	: m_hWnd(nullptr), m_pD3DDevice(nullptr), m_pCommandQueue(nullptr), m_pCommandAllocator(nullptr),
	m_pCommandList(nullptr), m_ui64enceValue(0), m_FeatureLevel(D3D_FEATURE_LEVEL_11_0),
	m_AdaptorDesc{}, m_pSwapChain(nullptr), m_pRenderTargets{}, 
	m_pRTVHeap(nullptr), m_pDSVHeap(nullptr), m_pSRVHeap(nullptr),
	m_rtvDescriptorSize(0), m_dwSwapChainFlags(0), m_uiRenderTargetIndex(0),
	m_hFenceEvent(nullptr), m_pFence(nullptr), m_dwCurContextIndex(0),
	m_Viewport{}, m_ScissorRect{},m_dwWidth(0),m_dwHeight(0)
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanUpRenderer();
}
