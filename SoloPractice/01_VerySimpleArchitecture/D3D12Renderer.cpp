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

	// window 정보 받고
	m_hWnd = _hWnd;

	// debug layer를 켜는데 사용하는 interface
	Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController = nullptr;
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

		// Window에 맞춰서 Viewport와 Scissor Rect를 정의한다.
		m_Viewport.Width = static_cast<float>(uiWndWidth);
		m_Viewport.Height = static_cast<float>(uiWndHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;

		m_ScissorRect.left = 0;
		m_ScissorRect.right = uiWndWidth;
		m_ScissorRect.top = 0;
		m_ScissorRect.bottom = uiWndHeight;
		// 맴버도 채운다.
		m_dwWidth = static_cast<DWORD>(uiWndWidth);
		m_dwHeight = static_cast<DWORD>(uiWndHeight);
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
	// 화면 클리어 및 이번 프레임 렌더링을 위한 자료구조 초기화

	// d3d12 에서는 화면 클리어 하는 것도 command list로 명령을 작성해야 한다.
	// 일단 allocator와 list를 초기화 해주고
	if (FAILED(m_pCommandAllocator->Reset())) {
		__debugbreak();
	}
	// 당장은 PSO가 없기 때문에 nullptr으로 초기화 한다.
	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
	}

	// 현재 백버퍼 인덱스에 맞는 RTV를 얻어온다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	// 그리고 RT Resource 위에 그릴 수 있게, PRESENT에서 RENDER_TARGET으로 바꿔준다.
	// (이것 역시 완전 비공기 API인 D3D12를 위해 Resource를 보호하는 방법이다.)
	D3D12_RESOURCE_BARRIER trans_PRESENT_RT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &trans_PRESENT_RT);

	// command를 기록한다.
	// RTV handle과 초기화 할 색을 넣어준다. (z 버퍼는 생략되었다.)
	m_pCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

	// viewport, scissor rect, render target을 설정해줘야
	// 그 위에 뭔가를 그릴 수 있다.
	m_pCommandList->RSSetViewports(1, &m_Viewport);
	m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
	// 현재도 z 버퍼는 생략되었다.
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void D3D12Renderer::EndRender()
{
	// 그릴 것을 다 그렸으니 이제 Render target의 상태를 ResourceBarrier 상태를 PRESENT로 바꾼다.
	D3D12_RESOURCE_BARRIER trans_RT_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &trans_RT_PRESENT);

	// 마지막에 close를 걸고
	m_pCommandList->Close();

	// queue로 넘겨준다. (여기까지의 과정을 매 프레임마다 하는 것이다.)
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get()};
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

}

void D3D12Renderer::Present()
{
	// back Buffer 화면을 Primary Buffer로 전송
	UINT SyncInterval = 1; // Vsync on , 0이면 Vsync를 off 하는 것이다.

	UINT uiSyncInterval = SyncInterval;
	UINT uiPresentFlags = 0;

	// 모니터의 주사율과 GPU Rendering 주기와의 차이에서 생기는 화면이 찢어지는 현상이다.
	if (!uiSyncInterval) {
		// 이렇게 해야 Tearing(화면 찢어짐)을 무시하고
		// Vsync를 꺼준다.
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}
	// 백 버퍼와 프론트 버퍼를 바꾼다.
	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		__debugbreak();
	}

	// 백 버퍼로 바뀐 친구를 다음 프레임에 그릴 인덱스로 지정한다.
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 다음 프레임 작업을 하기 전에
	// 리소스 동기화를 위해 queue 작업을 GPU가 끝냈는지, fence로 확인한다.
	DoFence();

	WaitForFenceValue();
}

bool D3D12Renderer::UpdateWindowSize(DWORD _dwWidth, DWORD _dwHeight)
{
	// 유효하지 않은 크기나
	if (!(_dwWidth * _dwHeight)) {
		return false;
	}
	// 크기가 변하지 않았으면, 실행하지 않는다.
	if (_dwWidth == m_dwWidth && _dwHeight == m_dwHeight) {
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	// 스왑 체인 정보를 저장해놓고
	HRESULT hr = m_pSwapChain->GetDesc1(&desc);
	if (FAILED(hr)) {
		__debugbreak();
	}
	// 원래 있던 RT Resource는 해제해주고
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		m_pRenderTargets[i]->Release();
		m_pRenderTargets[i] = nullptr;
	}
	// 새로운 버퍼를 생성한다.
	hr = m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, _dwWidth, _dwHeight, DXGI_FORMAT_R8G8B8A8_UNORM, m_dwSwapChainFlags);
	if (FAILED(hr)) {
		__debugbreak();
	}
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 새로운 버퍼를 가리키는 Resource View를 생성한다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// 맴버도 업데이트 해준다.
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
	// 이렇게 형변환을 해야 문제가 안 생긴다. (메모리 크기 + 소멸자 호출)
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
	m_hFenceEvent(nullptr), m_pFence(nullptr), m_dwCurContextIndex(0),
	m_Viewport{}, m_ScissorRect{},m_dwWidth(0),m_dwHeight(0)
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanUpRenderer();
}
