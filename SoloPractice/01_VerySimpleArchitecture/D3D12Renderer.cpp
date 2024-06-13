// D3D12Renderer.cpp from "megayuchi"

#include "pch.h"
#include "D3D12Renderer.h"
#include <dxgidebug.h>
#include "D3DUtil.h"

bool D3D12Renderer::Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV)
{
	bool bResult = false;
	HRESULT hr = S_OK;

	// window 정보 받고
	m_hWnd = _hWnd;

	// debug layer를 켜는데 사용하는 interface
	Microsoft::WRL::ComPtr<ID3D12Debug5> pDebugController = nullptr;
	// DXGI 개체를 생성하는 interface
	Microsoft::WRL::ComPtr<IDXGIFactory7> pFactory = nullptr;
	// display subsystem의 스펙을 알아내는 interface
	Microsoft::WRL::ComPtr<IDXGIAdapter4> pAdaptor = nullptr;

	DXGI_ADAPTER_DESC3 AdaptorDesc = {};

	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;

	// #1 GPU 디버그 레이어 설정
	if (_bEnableDebugLayer) {
		// 원래는 엄청 복잡한 모양으로 interface를 얻기가 어려웠는데,
		// 아래 헬퍼함수로 엄청 편해졌다고 한다.
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
		if (SUCCEEDED(hr)) {
			pDebugController->EnableDebugLayer();
		}

		// #2 GBV(GPU based Validation) 설정
		// 아래는 DXGI에 대해서 디버깅 플래그를 설정해주는 것이다.
		dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		// D3D12는 CPU 타임라인과, GPU 타임라인이 다르다.
		// (driver에서 돌아가는 것, shader에서 돌아가는 것이 다르다.)

		// 어쨌든 여기서 하는건, GPU에서 뭔가 문제가 생겨서 터질때 잡아준다는 것이다.
		if (_bEnableGBV) {
			// debug6라는 query interface를 가져온다.
			Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController6 = nullptr;
			if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(pDebugController6.GetAddressOf()))) {
				// 그 interface로 GBV를 켜준다.
				pDebugController6->SetEnableGPUBasedValidation(TRUE);
				pDebugController6->SetEnableAutoName(TRUE);
			}
		}
	}

	// #3 DXGIFactory 생성
	// 옛날 DirectX는 두개(d3d + dxgi)가 하나에 있었는데, 그걸 2개로 분리했다.

	// dxgi를 사용하는 이유는 double-buffering을 하기 위함이다.
	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory));

	// GPU가 가지고 있는 기능들의 수준을 나타내는 것
	// (예전에는 GPU가 DirectX 표준에 맞춰서 기능을 제공하지 않았다. 그래서 모든 기능을 다 확인해야했다.)
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1, // 여기부터 레이트레이싱을 지원한다.
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0, // 기본 기능은 여기로도 충분하다.
	};

	DWORD FeatureLevelNum = _countof(featureLevels);

	// #4 GPU feature level에 맞는 D3DDevice 생성
	for (DWORD flIndex = 0; flIndex < FeatureLevelNum; flIndex++) {
		UINT adaptorIndex = 0;
		// DXGI가 가진 기능중에 그래픽 카드를 연결하는 기능도 있다.
		// DXGIFactory에서 어뎁터를 얻어와서
		IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(pAdaptor.GetAddressOf());
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adaptorIndex, pTempAdaptor)) {
			pAdaptor->GetDesc3(&AdaptorDesc);
			// 그래픽 카드들을 확인해보면서 피쳐 레벨을 확인하는 것이다.
			// GPU에다가 D3DDevice를 생성해보고 성공하면 해당 feature level을 가지고 있는 것이다.
			if (SUCCEEDED(D3D12CreateDevice(pAdaptor.Get(), featureLevels[flIndex], IID_PPV_ARGS(&m_pD3DDevice)))) {
				goto EXIT;
			}
			// 형변환을 해서 그런가.. 스마트 포인터로 해제가 안된다.
			(*pTempAdaptor)->Release();
			(*pTempAdaptor) = nullptr;
			adaptorIndex++;
		}
	}
EXIT:
	// d3ddevice를 생성하고
	if (!m_pD3DDevice) {
		__debugbreak();
		goto RETURN;
	}
	m_pD3DDevice->SetName(L"device");
	m_AdaptorDesc = AdaptorDesc;

	// 참고 : goto 아래에 이렇게 바디를 새로 만드는 이유는 goto 때문에, 분기가 될때는 바디가 있어야 변수를 초기화 할 수 있다. -> "컨트롤 전송"

	// d3d debug 추가 설정을 할 수 있다.
	if (pDebugController) {
		// D3DUtil.h
		SetDebugLayerInfo(m_pD3DDevice.Get());
	}

	// #5 매 프레임 마다 작성한 Command List가 올라갈 Command Queue를 생성
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
	// #6 Descriptor Heap을 생성한다.
	CreateDescriptorHeap();

	// #7 swap chain과 그에 필요한 ID3DResource를 만든다.

	// swapchain 속성을 지정하고
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
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // 티어링 현상을 막지 않는다.
		
		m_dwSwapChainFlags = swapChainDesc.Flags;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain1 = nullptr;
		// DXGI와 윈도우 핸들을 가지고 swapchain을 만든다.
		if (FAILED(pFactory->CreateSwapChainForHwnd(
			m_pCommandQueue.Get(), _hWnd, &swapChainDesc, &fsSwapChainDesc,
			nullptr, pSwapChain1.GetAddressOf()
		))) 
		{
			__debugbreak();
		}
		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	}

	// #8 각각의 Frame에 대해 Frame Resource를 만든다.
	{

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
			// Descriptor Heap에 Render Target 역할을 하는 SwapChain의 프론트 버퍼를
			// ID3D12Resource으로 가져와서
			m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
			// 그리고 그것을 가리키는 view(일종의 포인터)를 설정함으로 렌더타겟으로 쓸 수 있게 한다.
			m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
			// RTV Heap handle을 다음으로 옮겨 백 버퍼도 똑같이 설정한다.
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			m_pRenderTargets[i]->SetName(L"Render Target Resource");
		}
	}

	// #9 Command List를 만든다.
	CreateCommandList();

	// #10 fence를 정의한다.
	// synchronization objects가 필요한 이유는, d3d12는 GPU에서 리소스를 사용하기 전에
	// 그것을 해제해 버릴 수 있다. 그래서 이렇게 fence를 쳐줘서 없애기 전에 확인 해준다.
	// d3d12는 완전 비둥기(asynchronous) api다.
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
}

void D3D12Renderer::EndRender()
{
}

void D3D12Renderer::Present()
{
}

void D3D12Renderer::CreateCommandList()
{
	// command list는 command allocator와 command list로 나뉘어져 있다.
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandAllocator->SetName(L"Command Allocator");
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandList->SetName(L"Command List");
	// 일단 처음에는 close를 해준다.
	m_pCommandList->Close();
}

bool D3D12Renderer::CreateDescriptorHeap()
{
	HRESULT hr = S_OK;

	// Render Target용 Descriptor heap을 만든다.
	// Render Target도 ID3D12Resource를 사용한다.
	// 그래서 RT도 GPU가 사용할 버퍼(물리 메모리)를 만들어줘야 하는 것이다.
	// (Descriptor Heap을 참조하는 것은 Descriptor Table이다.)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // render target에 맞는 타입을 지정해준다.
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf())))) {
		__debugbreak();
	}
	m_pRTVHeap->SetName(L"Render Target Heap");
	// Render Target view의 offset stride size를 저장해 놓는다.
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
	// 해당 fence까지 GPU가 작업을 마쳤는지는 window event로 처리한다.
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
	// fence를 기다린다.
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue) {
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3D12Renderer::CleanUpRenderer()
{
	// 혹시 남아있을 GPU 작업을 마무리 한다.
	WaitForFenceValue();

	CleanupFence();
}

D3D12Renderer::D3D12Renderer()
	: m_hWnd(nullptr), m_pD3DDevice(nullptr), m_pCommandQueue(nullptr), m_pCommandAllocator(nullptr),
	m_pCommandList(nullptr), m_ui64enceValue(0), m_FeatureLevel(D3D_FEATURE_LEVEL_11_0),
	m_AdaptorDesc{}, m_pSwapChain(nullptr), m_pRenderTargets{}, 
	m_pRTVHeap(nullptr), m_pDSVHeap(nullptr), m_pSRVHeap(nullptr),
	m_rtvDescriptorSize(0), m_dwSwapChainFlags(0), m_uiRenderTargetIndex(0),
	m_hFenceEvent(nullptr), m_pFence(nullptr), m_dwCurContextIndex(0)
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanUpRenderer();
}
