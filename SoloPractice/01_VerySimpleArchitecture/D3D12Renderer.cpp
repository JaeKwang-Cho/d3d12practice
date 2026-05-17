// D3D12Renderer.cpp from "megayuchi"

#include "pch.h"
#include "D3D12Renderer.h"
#include "D3DUtil.h"
#include "BasicMeshObject.h"
#include "D3D12ResourceManager.h"
#include "RenderThread.h"
#include "DescriptorPool.h"
#include "SingleDescriptorAllocator.h"
#include "ConstantBufferManager.h"
#include "SpriteObject.h"
#include "D3D12PSOCache.h"
#include "ColorRenderMesh.h"
#include "FlyCamera.h"
#include "TextureRenderMesh.h"
#include "ScreenStreamer.h"
#include "FontManager.h"
#include "TextureManager.h"
#include "Grid_RenderMesh.h"
#include "VertexUtil.h"
#include "RenderQueue.h"
#include "IRenderMesh.h"
#include "CommandListPool.h"
#include <filesystem>

#define PIXEL_STREAMING (0)

namespace
{
	std::wstring NormalizeAbsolutePath(const WCHAR* _wchPath)
	{
		if (_wchPath == nullptr || _wchPath[0] == L'\0')
		{
			return std::wstring();
		}

		std::filesystem::path path(_wchPath);
		if (!path.is_absolute())
		{
			path = std::filesystem::absolute(path);
		}

		return path.lexically_normal().wstring();
	}

	std::wstring ResolvePathWithRoot(const std::wstring& _strRootPath, const WCHAR* _wchPath)
	{
		if (_wchPath == nullptr || _wchPath[0] == L'\0')
		{
			return std::wstring();
		}

		std::filesystem::path path(_wchPath);
		if (path.is_absolute())
		{
			return path.lexically_normal().wstring();
		}

		if (!_strRootPath.empty())
		{
			return (std::filesystem::path(_strRootPath) / path).lexically_normal().wstring();
		}

		return std::filesystem::absolute(path).lexically_normal().wstring();
	}
}

void D3D12Renderer::SetAssetRootPath(const WCHAR* _wchAssetRootPath)
{
	m_strAssetRootPath = NormalizeAbsolutePath(_wchAssetRootPath);
}

void D3D12Renderer::SetShaderRootPath(const WCHAR* _wchShaderRootPath)
{
	m_strShaderRootPath = NormalizeAbsolutePath(_wchShaderRootPath);
}

std::wstring D3D12Renderer::ResolveAssetPath(const WCHAR* _wchPath) const
{
	return ResolvePathWithRoot(m_strAssetRootPath, _wchPath);
}

std::wstring D3D12Renderer::ResolveShaderPath(const WCHAR* _wchPath) const
{
	return ResolvePathWithRoot(m_strShaderRootPath, _wchPath);
}

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

	m_fDPI = static_cast<float>(GetDpiForWindow(m_hWnd));

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
		return false;
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
			return false;
		}
	}
	m_pCommandQueue->SetName(L"Command Queue");
	// #6 RTV용 Descriptor Heap을 생성한다.
	CreateDescriptorHeapForRTV();

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
		hr = pFactory->CreateSwapChainForHwnd(
			m_pCommandQueue.Get(), _hWnd, &swapChainDesc, &fsSwapChainDesc,
			nullptr, pSwapChain1.GetAddressOf());
		if (FAILED(hr))
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
		m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// #9 depth-stencil 전용 Heap과 그위에 resource view를 만든다.
	CreateDescriptorHeapForDSV();
	CreateDepthStencil(m_dwWidth, m_dwHeight);

	// #11 fence를 정의한다.
	// synchronization objects가 필요한 이유는, d3d12는 GPU에서 리소스를 사용하기 전에
	// 그것을 해제해 버릴 수 있다. 그래서 이렇게 fence를 쳐줘서 없애기 전에 확인 해준다.
	// d3d12는 완전 비둥기(asynchronous) api다.
	CreateFence();

	// Resource Manager
	m_pResourceManager = std::make_unique<D3D12ResourceManager>();
	m_pResourceManager->Initialize(m_pD3DDevice);

	// Font Manager
	m_pFontManager = std::make_unique<FontManager>();
	m_pFontManager->Initialize(this, m_pCommandQueue.Get(), 1024, 256, _bEnableDebugLayer);

	// Texture Manager
	m_pTextureManager = std::make_unique<TextureManager>();
	m_pTextureManager->Initalize(this);

#if USE_MULTI_THREAD_RENDERING
	ULONG dwPhysicalCoreCount = 0;
	{ // GetLogicalProcessorInformationEx 함수를 이용해서 물리 코어의 개수를 구한다.
		DWORD len = 0;
		GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &len);

		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer =
			(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(len);

		if (!GetLogicalProcessorInformationEx(RelationProcessorCore, buffer, &len)) {
			free(buffer);
		}

		BYTE* ptr = (BYTE*)buffer;
		BYTE* end = ptr + len;

		while (ptr < end) {
			SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info =
				(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)ptr;

			if (info->Relationship == RelationProcessorCore)
				dwPhysicalCoreCount++;

			ptr += info->Size;
		}
		free(buffer);
	}
	m_ulRenderThreadCount = dwPhysicalCoreCount;
	if (m_ulRenderThreadCount > MAX_RENDER_THREAD_COUNT)
		m_ulRenderThreadCount = MAX_RENDER_THREAD_COUNT;

	InitRenderThreadPool();
#else
	m_ulRenderThreadCount = 1;
#endif

	// Render Queue
	for (ULONG i = 0; i < m_ulRenderThreadCount; i++) {
		m_pRenderQueue[i] = std::make_unique<RenderQueue>();
		m_pRenderQueue[i]->Initialize(this, 8192); // 8192개의 draw call이 한 프레임에 들어올 수 있다고 가정한다.
	}

	// Command List당 pool도 각각 만들어준다.
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		for (ULONG j = 0; j < m_ulRenderThreadCount; j++) {
			// Constant Buffer Pool
			m_ppConstantBufferManager[i][j] = std::make_unique<ConstantBufferManager>();
			m_ppConstantBufferManager[i][j]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME);
			// Descriptor Pool
			m_ppDescriptorPool[i][j] = std::make_unique<DescriptorPool>();
			m_ppDescriptorPool[i][j]->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME * BasicMeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW); // draw call 한 번당 Descriptor 하나가 넘어간다.
			// Command List Pool
			m_ppCommandListPool[i][j] = std::make_unique<CommandListPool>();
			m_ppCommandListPool[i][j]->Initialize(m_pD3DDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, 256);
		}
	}
	// SingleDescriptorAllocator
	m_pSingleDescriptorAllocator = std::make_unique<SingleDescriptorAllocator>();
	m_pSingleDescriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPRTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	// PSOCache
	m_pD3D12PSOCache = std::make_unique<D3D12PSOCache>();
	m_pD3D12PSOCache->Initialize(this);

	// Camera
	InitCamera();

	// Frame CBV
	InitFrameCB();

	// Init Grid
	{
		m_pGridRenderMesh = std::make_unique<Grid_RenderMesh>();
		std::vector<ColorMeshData> gridData = CreateTileGrid();
		m_pGridRenderMesh->Initialize(this, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		m_pGridRenderMesh->CreateRenderAssets(gridData, 1);
	}

	// Init Common Resources
	{
		//m_pDefaultWhiteTexture = CreateTextureFromFile(L"../../Assets/white.png");
		m_pDefaultWhiteTexture = CreateTextureFromFile(L"white.png");
		m_DefaultMaterial = CONSTANT_BUFFER_MATERIAL();
	}

#if PIXEL_STREAMING
	// ScreenCapturer
	m_pScreenStreamer = std::make_unique<ScreenCapturer>();
	m_pScreenStreamer->Initialize(this, m_pRenderTargets[0]->GetDesc());
#endif

	bResult = true;
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

void D3D12Renderer::Update(const GameTimer& _gameTimer)
{
	// 뭔가 업데이트를 해보자
	// 입력이라던가..
	OnKeyboardInput_Renderer(_gameTimer);
	
	// 카메라 같은것들
	m_flyCamera->UpdateViewMatrix();

	// 그리고 Frame 당 하나씩 가지고 있는 CBV를 업데이트하기
	UpdateFrameCB();
}

void D3D12Renderer::BeginRender()
{
	// 화면 클리어 및 이번 프레임 렌더링을 위한 자료구조 초기화

	// 현재 Rendering을 할 Command Allocator와 List에 대해서 초기화를 진행한다.
	/*
	D3D12CommandAllocator_ptr pCommandAllocator = m_ppCommandAllocator[m_dwCurContextIndex];
	ifMicrosoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList = m_ppCommandList[m_dwCurContextIndex]; (FAILED(pCommandAllocator->Reset())) {
		__debugbreak();
	}
	// 당장은 PSO가 없기 때문에 nullptr으로 초기화 한다.
	if (FAILED(pCommandList->Reset(pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
	}

	*/
	// pool 에서 가져오기
	CommandListPool* pCommandListPool = m_ppCommandListPool[m_dwCurContextIndex][0].get();
	D3D12GraphicsCommandList_raw pCommandList = pCommandListPool->GetCurrentCommandList();

	//RT Resource 위에 그릴 수 있게, PRESENT에서 RENDER_TARGET으로 바꿔준다.
	// (이것 역시 완전 비공기 API인 D3D12를 위해 Resource를 보호하는 방법이다.)
	D3D12_RESOURCE_BARRIER trans_PRESENT_RT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &trans_PRESENT_RT);

	// 현재 백버퍼 인덱스에 맞는 RTV를 얻어온다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	// DSV도 얻어온다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// command를 기록한다.
	// RTV handle과 초기화 할 색을 넣어준다.
	pCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);
	// DSV도 적절히 초기화 해준다.
	pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// 즉시 실행
	pCommandListPool->CloseAndExecuteCurrentCommandList(m_pCommandQueue.Get());

	DoFence();
}

void D3D12Renderer::EndRender()
{
	CommandListPool* pCommandListPool = m_ppCommandListPool[m_dwCurContextIndex][0].get();


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

#ifdef USE_MULTI_THREAD_RENDERING
	//  Tell each render thread to process its render items and wait for them to finish.
	for(ULONG i = 1; i < m_ulRenderThreadCount; i++)
	{
		m_RenderThreadDescs[i].ProcessSignal.release();
	}
	ProcessByThread(0);
	for(ULONG i = 1; i < m_ulRenderThreadCount; i++)
	{
		m_RenderThreadDescs[i].FinishSignal.acquire();
	}

	// Wait for all render threads to finish processing their render items before we present the frame.

#else
	ProcessByThread(0);
#endif

	// 그릴 것을 다 그렸으니 이제 Render target의 상태를 ResourceBarrier 상태를 PRESENT로 바꾼다.
	D3D12_RESOURCE_BARRIER trans_RT_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	D3D12GraphicsCommandList_raw pCommandList = pCommandListPool->GetCurrentCommandList();
	pCommandList = pCommandListPool->GetCurrentCommandList();
	pCommandList->ResourceBarrier(1, &trans_RT_PRESENT);

	pCommandListPool->CloseAndExecuteCurrentCommandList(m_pCommandQueue.Get());

	for(ULONG i = 0; i< m_ulRenderThreadCount; i++)
	{
		m_pRenderQueue[i]->ResetQueue();
	}
}

void D3D12Renderer::Present()
{
	// fence 설정을 현재 CommandList에 대해서 한다.
	DoFence();

	// back Buffer 화면을 Primary Buffer로 전송
	UINT SyncInterval = 0; // Vsync on , 0이면 Vsync를 off 하는 것이다.
	// 이걸 키면 프레임이 느려진다.

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
	UINT uiRTIndexToCopy = m_uiRenderTargetIndex;
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 다음 프레임 작업을 하기 전에, 다음 렌더링할 CommandList에 해당하는 Fence값이 만족했는지 확인한다.
	DWORD dwNextContextIndex = (m_dwCurContextIndex + 1) & MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_pui64LastFenceValue[dwNextContextIndex]);

#if PIXEL_STREAMING
	// fence 값이 만족했으면, copy도 완료된 것이니 send를 건다.
	// 여기서도 Thread가 작업 중이였으면 스킵하는거로 한다.
	if (bCheckUpdateTexture == true && m_pScreenStreamer->CheckSendable() == true)
	{
		m_pScreenStreamer->SendPixelsFromTexture();
	}
	bCheckUpdateTexture = false;
	bTryPixelStreaming = false;
	//m_pScreenStreamer->CreatFileFromTexture(uiRTIndexToCopy); // 임시 코드
#endif

	// 한 프레임이 끝났으니 0으로 초기화 한다.
	for(ULONG i = 0; i < m_ulRenderThreadCount; i++)
	{
		m_ppConstantBufferManager[dwNextContextIndex][i]->Reset();
		m_ppDescriptorPool[dwNextContextIndex][i]->Reset();
		m_ppCommandListPool[dwNextContextIndex][i]->ResetCommandListPool();
	}
	m_dwCurContextIndex = dwNextContextIndex;
 }

bool D3D12Renderer::UpdateWindowSize_Renderer(DWORD _dwWidth, DWORD _dwHeight)
{
	// 유효하지 않은 크기나
	if (!(_dwWidth * _dwHeight)) {
		return false;
	}
	// 크기가 변하지 않았으면, 실행하지 않는다.
	if (_dwWidth == m_dwWidth && _dwHeight == m_dwHeight) {
		return false;
	}

	// 모든 CommandList에 대해서 wait을 건다.
	DoFence();
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	// 스왑 체인 정보를 저장해놓고
	HRESULT hr = m_pSwapChain->GetDesc1(&desc);
	if (FAILED(hr)) {
		__debugbreak();
	}
	// 원래 있던 RT Resource는 해제해주고
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		m_pRenderTargets[i] = nullptr;
		// ComPtr은 static이 아니면 Release를 안하는게 좋은듯
	}
	// DS Resource도 해제해준다.
	m_pDepthStencil = nullptr;

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

	CreateDepthStencil(_dwWidth, _dwHeight);

	// 맴버도 업데이트 해준다.
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

IRenderMesh* D3D12Renderer::CreateTextureRenderMesh(const TextureMeshData& _mesh, const std::vector<std::uint32_t>& _adjIndices, const std::vector<SubmeshRange>& _ranges)
{
	TextureRenderMesh* pMeshObj = new TextureRenderMesh;
	if (!pMeshObj->Initialize(this))
	{
		delete pMeshObj;
		__debugbreak();
		return nullptr;
	}

	if (!pMeshObj->CreateRenderAssetsFromSingleMesh(_mesh, _adjIndices, _ranges))
	{
		delete pMeshObj;
		__debugbreak();
		return nullptr;
	}

	return pMeshObj;
}

void D3D12Renderer::DeleteRenderMesh(IRenderMesh* _pMeshObjectHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	delete _pMeshObjectHandle;
}

void D3D12Renderer::BindTextureToMesh(IRenderMesh* _pMeshObjectHandle, TEXTURE_HANDLE* _pTexHandle, UINT _subRenderAssetIndex)
{
	if (_pMeshObjectHandle == nullptr || _pTexHandle == nullptr)
	{
		__debugbreak();
		return;
	}

	TextureRenderMesh* pMeshObj = dynamic_cast<TextureRenderMesh*>(_pMeshObjectHandle);
	if (pMeshObj == nullptr)
	{
		__debugbreak();
		return;
	}

	pMeshObj->BindTextureAssets(_pTexHandle, _subRenderAssetIndex);
}

void D3D12Renderer::SetMeshMaterial(IRenderMesh* _pMeshObjectHandle, const CONSTANT_BUFFER_MATERIAL& _MaterialData, UINT _subRenderAssetIndex)
{
	if (_pMeshObjectHandle == nullptr)
	{
		__debugbreak();
		return;
	}

	TextureRenderMesh* pMeshObj = dynamic_cast<TextureRenderMesh*>(_pMeshObjectHandle);
	if (pMeshObj == nullptr)
	{
		__debugbreak();
		return;
	}

	CONSTANT_BUFFER_MATERIAL material = _MaterialData;
	pMeshObj->SetMaterial(material, _subRenderAssetIndex);
}

void D3D12Renderer::DrawRenderMesh(IRenderMesh* _pMeshObjectHandle, const XMMATRIX* pMatWorld)
{
	RENDER_ITEM renderItem = {};
	renderItem.Type = RENDER_ITEM_TYPE::MESH;
	renderItem.MeshObjParam.pMesh = _pMeshObjectHandle;
	renderItem.MeshObjParam.matWorld = *pMatWorld;
	renderItem.MeshObjParam.Pass = RENDER_MESH_PASS::Default;

	AddItemToRenderQueue(renderItem);
}
void D3D12Renderer::DrawOutlineMesh(IRenderMesh* _pMeshObjectHandle, const XMMATRIX* _pMatWorld)
{
	RENDER_ITEM item = {};
	item.Type = RENDER_ITEM_TYPE::MESH;
	item.MeshObjParam.pMesh = _pMeshObjectHandle;
	item.MeshObjParam.matWorld = *_pMatWorld;
	item.MeshObjParam.Pass = RENDER_MESH_PASS::Outline;

	AddItemToRenderQueue(item);
}
void D3D12Renderer::DrawGrid()
{
	DrawRenderMesh(m_pGridRenderMesh.get(), &m_matGridWorld);
}
void D3D12Renderer::UpdateGridWorldMatrix(UINT _gridCellOffset)
{
	XMFLOAT3 curCameraPos = GetCameraWorldPos();

	float xSnapped = floorf(curCameraPos.x / _gridCellOffset) * _gridCellOffset;
	float zSnapped = floorf(curCameraPos.z / _gridCellOffset) * _gridCellOffset;

	m_matGridWorld = XMMatrixTranslation(xSnapped, 0.f, zSnapped);
}

SPRITE_HANDLE* D3D12Renderer::CreateSpriteObject()
{
	SPRITE_HANDLE* pSpriteHandle = new SPRITE_HANDLE;
	pSpriteHandle->pSpriteObject = new SpriteObject;

	if (!pSpriteHandle->pSpriteObject->Initialize(this))
	{
		delete pSpriteHandle->pSpriteObject;
		pSpriteHandle->pSpriteObject = nullptr;
		delete pSpriteHandle;
		__debugbreak();
		return nullptr;
	}

	return pSpriteHandle;
}
SPRITE_HANDLE* D3D12Renderer::CreateSpriteObject(const WCHAR* _wchTexFileName, int _posX, int _posY, int _width, int _height)
{
	SPRITE_HANDLE* pSpriteHandle = new SPRITE_HANDLE;
	pSpriteHandle->pSpriteObject = new SpriteObject;

	RECT rect = {};
	rect.left = _posX;
	rect.top = _posY;
	rect.right = _width;
	rect.bottom = _height;

	if (!pSpriteHandle->pSpriteObject->Initialize(this, _wchTexFileName, &rect))
	{
		delete pSpriteHandle->pSpriteObject;
		pSpriteHandle->pSpriteObject = nullptr;
		delete pSpriteHandle;
		__debugbreak();
		return nullptr;
	}

	return pSpriteHandle;
}

void D3D12Renderer::DeleteSpriteObject(SPRITE_HANDLE* _pSpriteObjHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	if (_pSpriteObjHandle == nullptr)
	{
		return;
	}

	delete _pSpriteObjHandle->pSpriteObject;
	_pSpriteObjHandle->pSpriteObject = nullptr;

	delete _pSpriteObjHandle;
}

void D3D12Renderer::RenderSpriteWithTex(SPRITE_HANDLE* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, const RECT* _pRect, float _z, TEXTURE_HANDLE* _pTexHandle)
{
	RENDER_ITEM item = {};
	item.Type = RENDER_ITEM_TYPE::SPRITE;
	item.SpriteParam.pSprite = _pSpriteObjHandle->pSpriteObject;
	item.SpriteParam.iPosX = _posX;
	item.SpriteParam.iPosY = _posY;
	item.SpriteParam.fScaleX = _scaleX;
	item.SpriteParam.fScaleY = _scaleY;

	if (_pRect) {
		item.SpriteParam.Rect = *_pRect;
		item.SpriteParam.bUseRect = true;
	}
	else {
		item.SpriteParam.Rect = {};
		item.SpriteParam.bUseRect = false;
	}

	item.SpriteParam.pTexHandle = _pTexHandle;
	item.SpriteParam.ZValue = _z;

	AddItemToRenderQueue(item);
}

void D3D12Renderer::RenderSprite(SPRITE_HANDLE* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, float _z)
{
	RENDER_ITEM item = {};
	item.Type = RENDER_ITEM_TYPE::SPRITE;
	item.SpriteParam.pSprite = _pSpriteObjHandle->pSpriteObject;
	item.SpriteParam.iPosX = _posX;
	item.SpriteParam.iPosY = _posY;
	item.SpriteParam.fScaleX = _scaleX;
	item.SpriteParam.fScaleY = _scaleY;
	item.SpriteParam.bUseRect = false;
	item.SpriteParam.Rect = {};
	item.SpriteParam.pTexHandle = nullptr;
	item.SpriteParam.ZValue = _z;
	
	AddItemToRenderQueue(item);
}

void D3D12Renderer::UpdateTextureWithImage(TEXTURE_HANDLE* _pTexHandle, const BYTE* _pSrcBytes, UINT _SrcWidth, UINT _SrcHeight)
{
	TEXTURE_HANDLE* pTexHandle = _pTexHandle;
	D3D12Resource_ptr pDestTexResource = pTexHandle->pTexResource;
	D3D12Resource_ptr pUploadBuffer = pTexHandle->pUploadBuffer;

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if(_SrcHeight > Desc.Height || _SrcWidth > Desc.Width)
	{
		__debugbreak();
		return;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
	UINT Rows = 0;
	UINT64 RowSize = 0;
	UINT64 TotalBytes = 0;

	m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, &Rows, &RowSize, &TotalBytes);

	BYTE* pMappedData = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	HRESULT hr = pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pMappedData));
	if(FAILED(hr)){
		__debugbreak();
		return;
	}

	const BYTE* pSrc = _pSrcBytes;
	BYTE* pDest = pMappedData;
	for(UINT y = 0; y < _SrcHeight; y++)
	{
		memcpy(pDest, pSrc, _SrcWidth * sizeof(BYTE) * 4);
		pSrc += _SrcWidth * sizeof(BYTE) * 4;
		pDest += RowSize;
	}
	pUploadBuffer->Unmap(0, nullptr);

	pTexHandle->bUpdated = true;
}

TEXTURE_HANDLE* D3D12Renderer::CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b)
{
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

	TEXTURE_HANDLE* pTexHandle = m_pTextureManager->CreateImmutableTexture_ITL(_texWidth, _texHeight, texFormat, pImage);

	free(pImage);
	pImage = nullptr;

	return pTexHandle;
}

TEXTURE_HANDLE* D3D12Renderer::CreateTextureFromFile(const WCHAR* _wchFileName)
{
	return m_pTextureManager->CreateTextureFromFile_ITL(ResolveAssetPath(_wchFileName).c_str());
}

TEXTURE_HANDLE* D3D12Renderer::CreateDynamicTexture(UINT _TexWidth, UINT _TexHeight)
{
	return m_pTextureManager->CreateDynamicTexture_ITL(_TexWidth, _TexHeight);
}

void D3D12Renderer::DeleteTexture(TEXTURE_HANDLE* _pTexHandle)
{
	// 뭔가 삭제할때는 이렇게 GPU 작업이 끝나기를 기다려야 한다.
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	m_pTextureManager->DeleteTexture_ITL((TEXTURE_HANDLE*)_pTexHandle);
}

FONT_HANDLE* D3D12Renderer::CreateFontObject(const WCHAR* _wchFontFamilyName, float _fFontSize)
{
	FONT_HANDLE* pFontHandle = m_pFontManager->CreateFontObject_ITL(_wchFontFamilyName, _fFontSize);
	return pFontHandle;
}

void D3D12Renderer::DeleteFontObject(FONT_HANDLE* _pFontHandle)
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}
	m_pFontManager->DeleteFontObject_ITL((FONT_HANDLE*)_pFontHandle);
}

bool D3D12Renderer::WriteTextToBitmap(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, FONT_HANDLE* _pFontHandle, const WCHAR* _wchString, DWORD _dwLen)
{
	bool bResult = m_pFontManager->WriteTextToBitmap_ITL(_pDestImage, _DestWidth, _DestHeight, _DestPitch, _piOutWidth, _piOutHeight, (FONT_HANDLE*)_pFontHandle, _wchString, _dwLen);
	return bResult;
}

D3D12PipelineState_raw D3D12Renderer::GetPSO(std::string _strPSOName)
{
	return m_pD3D12PSOCache->GetPSO(_strPSOName).Get();
}

bool D3D12Renderer::CachePSO(std::string _strPSOName, D3D12PipelineState_raw _pPSODesc)
{
	return m_pD3D12PSOCache->CachePSO(_strPSOName, _pPSODesc);
}

void D3D12Renderer::OnRButtonDown_Renderer(WPARAM _btnState, int _x, int _y)
{
	// 마우스 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hWnd);
}

void D3D12Renderer::OnRButtonUp_Renderer(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void D3D12Renderer::OnMouseMove_Renderer(WPARAM _btnState, int _x, int _y)
{
	// 왼쪽 마우스가 눌린 상태에서 움직인다면
	if ((_btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(_x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(_y - m_LastMousePos.y));

		m_flyCamera->AddPitch(dy);
		m_flyCamera->AddYaw(dx);
	}
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
}

void D3D12Renderer::OnKeyboardInput_Renderer(const GameTimer& _gameTimer)
{
	if (GetAsyncKeyState('W') & 0x8000)
		m_flyCamera->Walk(_gameTimer.GetDeltaTime() * 15.f);

	if (GetAsyncKeyState('S') & 0x8000)
		m_flyCamera->Walk(_gameTimer.GetDeltaTime() * - 15.f);

	if (GetAsyncKeyState('A') & 0x8000)
		m_flyCamera->Strafe(_gameTimer.GetDeltaTime() * -15.f);

	if (GetAsyncKeyState('D') & 0x8000)
		m_flyCamera->Strafe(_gameTimer.GetDeltaTime() * 15.f);

	if (GetAsyncKeyState('Q') & 0x8000)
		m_flyCamera->Ascend(_gameTimer.GetDeltaTime() * 15.f);

	if (GetAsyncKeyState('E') & 0x8000)
		m_flyCamera->Ascend(_gameTimer.GetDeltaTime() * -15.f);
}

ULONG D3D12Renderer::GetCommandListCount()
{
	ULONG ulCommandListCount = 0;
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		for (DWORD j = 0; j < m_ulRenderThreadCount; j++) {
			ulCommandListCount += m_ppCommandListPool[i][j]->GetTotalCommandListCount();
		}
		
	}
	return ulCommandListCount;
}

void D3D12Renderer::FlushMultiRendering()
{
	// 혹시나 작업중인 멀티렌더링 작업을 기다린다.
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}
}

bool D3D12Renderer::CreateDescriptorHeapForRTV()
{
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

bool D3D12Renderer::CreateDescriptorHeapForDSV()
{
	// Depth-Stencil용 DescriptorHeap을 만든다.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1; // 일단 기본으로 하나만 만든다.
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
	// Depth-Stencil 값을 저장할 Texture View를 생성한다.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
	dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsDesc.Flags = D3D12_DSV_FLAG_NONE;

	// 초기화할 값을 설정한다.
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

	// resource를 만들고
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

	// heap에 view로 올린다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pD3DDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &dsDesc, dsvHandle);

	return true;
}

void D3D12Renderer::InitCamera()
{
	if (!m_flyCamera) {
		m_flyCamera = new FlyCamera;

		m_flyCamera->SetPosition(XMFLOAT3(0.f, 3.f, -2.f));
		m_flyCamera->SetCameraLookAt(XMFLOAT3(0.f, 3.f, -2.f), XMFLOAT3(0.f, 2.9f, 1.f), XMFLOAT3(0.f, 1.f, 0.f));
		m_flyCamera->UpdateViewMatrix();
	}

	// FOV
	float fovY = XM_PIDIV4;

	// Projection
	float fAspectRatio = (float)m_dwWidth / (float)m_dwHeight;
	float fNear = 0.1f;
	float fFar = 1000.f;

	m_flyCamera->SetFrustum(fovY, fAspectRatio, fNear, fFar);
	// 가장 기본적인 카메라 설정으로 한다.
}

void D3D12Renderer::InitFrameCB()
{
	UINT alignedByteSize = static_cast<UINT>(AlignConstantBufferSize(sizeof(CONSTANT_BUFFER_FRAME)));
	D3D12_RESOURCE_DESC cbDesc_Size = CD3DX12_RESOURCE_DESC::Buffer(alignedByteSize);

	HRESULT hr = S_OK;
	for (int i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		hr = m_pD3DDevice->CreateCommittedResource(
			&HEAP_PROPS_UPLOAD,
			D3D12_HEAP_FLAG_NONE,
			&cbDesc_Size,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_ppFrameUploadCBs[i].GetAddressOf())
		);
		if (FAILED(hr)) {
			__debugbreak();
		}
		hr = m_ppFrameUploadCBs[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_ppFrameSystemMemAddrs[i]));
		if (FAILED(hr)) {
			__debugbreak();
		}
	}
}

void D3D12Renderer::UpdateFrameCB()
{
	CONSTANT_BUFFER_FRAME* pFrameCB = (CONSTANT_BUFFER_FRAME*)m_ppFrameSystemMemAddrs[m_dwCurContextIndex];

	pFrameCB->matProj = XMMatrixTranspose(m_flyCamera->GetProjMat());
	pFrameCB->matView = XMMatrixTranspose(m_flyCamera->GetViewMat());
	pFrameCB->matViewProj = XMMatrixMultiplyTranspose(m_flyCamera->GetViewMat(), m_flyCamera->GetProjMat());

	XMVECTOR detProj = XMMatrixDeterminant(m_flyCamera->GetProjMat());
	XMVECTOR detView = XMMatrixDeterminant(m_flyCamera->GetViewMat());
	XMMATRIX matViewProj = XMMatrixMultiply(m_flyCamera->GetViewMat(), m_flyCamera->GetProjMat());
	XMVECTOR detViewProj = XMMatrixDeterminant(matViewProj);

	pFrameCB->invProj = XMMatrixTranspose(XMMatrixInverse(&detProj, m_flyCamera->GetProjMat()));
	pFrameCB->intView = XMMatrixTranspose(XMMatrixInverse(&detProj, m_flyCamera->GetProjMat()));
	pFrameCB->invViewProj = XMMatrixTranspose(XMMatrixInverse(&detProj, m_flyCamera->GetProjMat()));

	pFrameCB->eyePosW = m_flyCamera->GetPosition();
	pFrameCB->renderTargetSize = XMFLOAT2((float)m_dwWidth, (float)m_dwHeight);
	pFrameCB->renderTargetSize_reciprocal = XMFLOAT2(1.f / m_dwWidth, 1.f / m_dwHeight);

	// light
	pFrameCB->ambientLight = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.f);
	float sunPhi = 1.25f * XM_PI;
	float sunTheta = XM_PIDIV4;

	pFrameCB->lights[0].direction = XMFLOAT3(
		-1.f * sinf(sunTheta) * cosf(sunPhi),
		-1.f * cosf(sunTheta),
		-1.f * sinf(sunTheta) * sinf(sunPhi)
	);
	pFrameCB->lights[0].strength = XMFLOAT3(1.f, 1.f, 1.f);
}

void D3D12Renderer::CreateFence()
{
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}
	m_pFence->SetName(L"Fence");
	m_ui64FenceValue = 0;
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
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	// 이렇게 Command list 별로 만족해야할 Fence 값을 따로따로 기록해준다.
	m_pui64LastFenceValue[m_dwCurContextIndex] = m_ui64FenceValue;
	return m_ui64FenceValue;
}

void D3D12Renderer::WaitForFenceValue(UINT64 _expectedFenceValue)
{
	// fence를 기다린다.
	if (m_pFence->GetCompletedValue() < _expectedFenceValue) {
		m_pFence->SetEventOnCompletion(_expectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3D12Renderer::CleanUpRenderer()
{
	// 혹시 남아있을 GPU 작업을 마무리 한다.
	DoFence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	CleanUpRenderThreadPool();

	ReleaseAllTextureHandles();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		for (DWORD j = 0; j < m_ulRenderThreadCount; j++) {
			// unique ptr이니까 소멸자에게 맡긴다.
		}

		if (m_ppFrameUploadCBs[i]) {
			m_ppFrameUploadCBs[i]->Unmap(0, nullptr);
			m_ppFrameUploadCBs[i] = nullptr;
		}
		m_ppFrameSystemMemAddrs[i] = nullptr;
	}

	if (m_pResourceManager) {
		m_pResourceManager.reset();
	}

	if (m_pSingleDescriptorAllocator) {
		m_pSingleDescriptorAllocator.reset();
	}

	if (m_pD3D12PSOCache) {
		m_pD3D12PSOCache.reset();
	}
	if(m_pFontManager)
	{
		m_pFontManager.reset();
	}
	if (m_flyCamera) {
		delete m_flyCamera;
		m_flyCamera = nullptr;
	}

	if (m_pScreenStreamer)
	{
		delete m_pScreenStreamer;
		m_pScreenStreamer = nullptr; 
	}

	CleanupFence();
}

void D3D12Renderer::ReleaseAllTextureHandles()
{
	m_pTextureManager.reset();
}

void D3D12Renderer::ProcessByThread(ULONG _ulThreadIndex)
{
	if (_ulThreadIndex >= m_ulRenderThreadCount) {
#if _DEBUG
		__debugbreak();
#endif
		return;
	}

	RenderQueue* pRenderQueue = m_pRenderQueue[_ulThreadIndex].get();
	CommandListPool* pCommandListPool = m_ppCommandListPool[m_dwCurContextIndex][_ulThreadIndex].get();

	if(!pRenderQueue || !pCommandListPool)
	{
		if (_ulThreadIndex != 0) {
			m_RenderThreadDescs[_ulThreadIndex].FinishSignal.release();
		}
#if _DEBUG
		__debugbreak();
#endif
		return;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	pRenderQueue->ProcessRenderItems(_ulThreadIndex, pCommandListPool, m_pCommandQueue.Get(), 400, rtvHandle, dsvHandle,  &m_Viewport, &m_ScissorRect);

	if(_ulThreadIndex != 0)
	{
		m_RenderThreadDescs[_ulThreadIndex].FinishSignal.release();
	}
}


bool D3D12Renderer::InitRenderThreadPool()
{
	m_ulCurThreadIndex = 0;

	for(ULONG i =0; i < m_ulRenderThreadCount; i++)
	{
		m_RenderThreadDescs[i].Renderer = this;
		m_RenderThreadDescs[i].uiThreadIndex = i;
	}

	// 0번은 메인 스레드 관리 한다.
	for(ULONG i = 1; i < m_ulRenderThreadCount; i++)
	{
		m_RenderThreadDescs[i].Thread = std::jthread(RenderThreadMain, &m_RenderThreadDescs[i]);
	}

	return true;
}

void D3D12Renderer::CleanUpRenderThreadPool()
{
	for(ULONG i = 0; i < m_ulRenderThreadCount; i++)
	{
		if (m_RenderThreadDescs[i].Thread.joinable())
		{
			m_RenderThreadDescs[i].Thread.request_stop();
			m_RenderThreadDescs[i].ProcessSignal.release();
			m_RenderThreadDescs[i].Thread.join();
		}
	}
}

void D3D12Renderer::AddItemToRenderQueue(const RENDER_ITEM& _RenderItem)
{
	m_pRenderQueue[m_ulCurThreadIndex]->AddRenderItem(_RenderItem);
	m_ulCurThreadIndex = (m_ulCurThreadIndex + 1) % m_ulRenderThreadCount;
}

D3D12Renderer::D3D12Renderer()
	: m_hWnd(nullptr), m_pD3DDevice(nullptr), m_pCommandQueue(nullptr), 
	m_pResourceManager(nullptr), m_ppConstantBufferManager{}, m_ppDescriptorPool{}, 
	m_pSingleDescriptorAllocator(nullptr), m_pD3D12PSOCache(nullptr),
	m_ppFrameUploadCBs{}, m_ppFrameSystemMemAddrs{},
	m_ui64FenceValue(0), m_pui64LastFenceValue{},
	m_FeatureLevel(D3D_FEATURE_LEVEL_11_0),
	m_AdaptorDesc{}, m_pSwapChain(nullptr), m_pRenderTargets{}, m_pDepthStencil(nullptr),
	m_pRTVHeap(nullptr), m_pDSVHeap(nullptr), m_pSRVHeap(nullptr),
	m_rtvDescriptorSize(0), m_srvDescriptorSize(0), m_dsvDescriptorSize(0),
	m_dwSwapChainFlags(0), m_uiRenderTargetIndex(0),
	m_hFenceEvent(nullptr), m_pFence(nullptr), m_dwCurContextIndex(0),
	m_Viewport{}, m_ScissorRect{},m_dwWidth(0),m_dwHeight(0),
	m_flyCamera(nullptr), m_LastMousePos{},
	m_pScreenStreamer(nullptr), bTryPixelStreaming(false), bCheckUpdateTexture(false),
	m_pTextureManager(nullptr), m_pFontManager(nullptr), m_pGridRenderMesh(nullptr), 
	m_matGridWorld{}, m_pRenderQueue{}, m_ulRenderThreadCount(0), m_ulCurThreadIndex(0),
	m_RenderThreadDescs{}, m_ppCommandListPool{}, m_strAssetRootPath(), m_strShaderRootPath()
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanUpRenderer();
}

D3D12ResourceManager* D3D12Renderer::INL_GetResourceManager()
{
	return m_pResourceManager.get();
}

ConstantBufferPool* D3D12Renderer::INL_GetConstantBufferPool(E_CONSTANT_BUFFER_TYPE _type, ULONG _ulThreadIndex)
{
	ConstantBufferManager* pConstBufferManager = m_ppConstantBufferManager[m_dwCurContextIndex][_ulThreadIndex].get();
	ConstantBufferPool* pConstBufferPool = pConstBufferManager->GetConstantBufferPool(_type);
	return pConstBufferPool;
}

DescriptorPool* D3D12Renderer::INL_GetDescriptorPool(ULONG _ulThreadIndex)
{
	return m_ppDescriptorPool[m_dwCurContextIndex][_ulThreadIndex].get();
}

SingleDescriptorAllocator* D3D12Renderer::INL_GetSingleDescriptorAllocator()
{
	return m_pSingleDescriptorAllocator.get();
}

D3D12PSOCache* D3D12Renderer::INL_GetD3D12PSOCache()
{
	return m_pD3D12PSOCache.get();
}

void D3D12Renderer::GetViewProjMatrix(XMMATRIX* _pOutMatView, XMMATRIX* _pOutMatProj)
{
	*_pOutMatView = m_flyCamera->GetViewMat();
	*_pOutMatProj = m_flyCamera->GetProjMat();
}

XMFLOAT3 D3D12Renderer::GetCameraWorldPos() const
{
	return m_flyCamera->GetPosition();
}

