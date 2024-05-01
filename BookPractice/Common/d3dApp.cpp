//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 메시지 루프에서 사용하는 프로시져 함수를 여기서 준다.
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::m_App = nullptr;

D3DApp* D3DApp::GetApp()
{
    return m_App;
}

D3DApp::D3DApp(HINSTANCE hInstance)
	:m_hAppInst(hInstance)
	,m_ScissorRect{}
	,m_ScreenViewport{}
{
    // D3DApp은 하나만 초기화
    assert(m_App == nullptr);
    m_App = this;
}

D3DApp::~D3DApp()
{
	if (m_d3dDevice != nullptr)
	{
		// 없애기 전에 Command Queue를 비우기
		FlushCommandQueue();
	}
}

HINSTANCE D3DApp::AppInst()const
{
	return m_hAppInst;
}

HWND D3DApp::MainWnd()const
{
	return m_hMainWnd;
}

float D3DApp::GetAspectRatio()const
{
	// 화면 종횡비
	return static_cast<float>(m_ClientWidth) / m_ClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
    return m_4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
    if(m_4xMsaaState != value)
    {
        m_4xMsaaState = value;

        // 새로운 멀티 샘플링이면 스왑체인을 다시 만들어야 한다.
        CreateSwapChain();
        OnResize();
    }
}

int D3DApp::Run()
{
	MSG msg = {0};
 
	m_Timer.Reset();

	while(msg.message != WM_QUIT)
	{
		// D3DApp 클래스 안에서 메시지 루프를 돌린다.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// 여기는 렌더링 부분
		else
        {	
			m_Timer.Tick();

			if( !m_AppPaused )
			{
				CalculateFrameStats();
				Update(m_Timer);
                Draw(m_Timer);
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	// View 를 초기에도 설정해줄 필요가 있다.
	OnResize();

	return true;
}
 
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// ==================   #6   ==================
	// Descriptor(View) Heap 설정과 생성
	// ============================================

	// 렌더타겟 힙
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, 
		IID_PPV_ARGS(m_RtvHeap.GetAddressOf())
	));

	// 뎁스 스텐실 힙
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};

    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())));
}

void D3DApp::OnResize()
{
	// ======================================
	//7번 8번 친구들은 CommandList로 해줘야한다.
	// ======================================

	assert(m_d3dDevice);
	assert(m_SwapChain);
    assert(m_CommandAllocator);

	// GPU에 넘어가 있는 Resource를 다시 그리고 싶다면, Flush를 해준다.
	FlushCommandQueue();

	// Command List reset하고
    ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	// 버퍼도 reset을 해버리고
	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		m_SwapChainBuffer[i].Reset();
	}
    m_DepthStencilBuffer.Reset();
	
	// Swap Chain의 크기를 다시 지정해준다.
    ThrowIfFailed(m_SwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		m_ClientWidth, 
		m_ClientHeight,
		m_BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_CurrBackBuffer = 0;


	// ==================   #7   ==================
	// RenderTarget View (Descriptor) 설정과 생성
	// ============================================
 
	// CD3DX12_CPU_DESCRIPTOR_HANDLE을 이용해서 좀 더 편하게(?)
	// 도우미 CPU Descriptor 구조체를 0번으로 초기화 한다.
	// 후면 버퍼는 CPU 자원이고, 이것을 View로 만들어서 GPU에게 넘겨줘야 하는 것

	// Render target heap의 시작 handle을가져오고
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// heap handle과 Buffer로 view(descriptor)을 만든다.
		// 스왑체인에서 버퍼를 얻고
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffer[i])));
		// 해당버퍼에 대한 render target view를 만들고
		m_d3dDevice->CreateRenderTargetView(m_SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// 힙 핸들을 다음 번호로 넘긴다.
		rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
	}

	// ==================   #8   ==================
	// Depth-Stencil View (Descriptor) 설정과 생성
	// ============================================

	// 얘는 GPU 자원들이여서, GPU 힙에 존재한다.
	// 그래서 함수 동작도 CPU 자원들과 다르다.

    // 얘는 Depth-Stencil View를 만드는 것이다.
    D3D12_RESOURCE_DESC depthStencilDesc = {};
	// Render Target과 똑같은 속성을 만들어주고
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_ClientWidth;
    depthStencilDesc.Height = m_ClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO 챕터는 depth buffer에서 값을 읽으려면 Shader Resource View Format을 depth buffer에게 요구한다.
	// (SSAO chapter requires an SRV to the depth buffer to read from
	// 그래서, 똑같은 resource에서 2개의 view를 만들어야 한다.
	// (Therefore, because we need to create two views to the same resource)
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// 그래서 일단 TYPELESS에서 depth buffer resource를 생성을 한다.
	// (we need to create the depth buffer resource with a typeless format.)  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	// DXGI_SAMPLE_DESC
    depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;

	// UKnown + Depth Stencil로 넘겨준다.
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 자원 지우기 설정을 나타내는 구조체 CLAER_VALUE
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = m_DepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	// 일단 생성하는 것은 Default 값으로 만들어 준다.
	CD3DX12_HEAP_PROPERTIES cHeapProps(D3D12_HEAP_TYPE_DEFAULT);

	// 이게 GPU 자원 전용으로 Resource를 생성하는 함수이다.
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &cHeapProps,
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(m_DepthStencilBuffer.GetAddressOf())));

	// Depth-Stencil View를 만들 구조체를 채워주고
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0; // 얘는 mip level을 0으로 해준다.

	// Resource와 구조체로 View를 만든다.
    m_d3dDevice->CreateDepthStencilView(
		m_DepthStencilBuffer.Get(), 
		&dsvDesc, 
		GetDepthStencilView());

	// 이제 GPU에 넘겨줄 수 있는 상태(Resource States)로 만든다.
	// ResourceBarrier 구조체로 관리를 해준다.

	// D3D12_RESOURCE_STATE_DEPTH_WRITE 상태
	D3D12_RESOURCE_BARRIER DepthBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_DepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	// 커맨드 리스트에서 리소스 베리어를 관리한다.
	m_CommandList->ResourceBarrier(
		1, 
		&DepthBarrierTransition
	);
	
    // 이제 Command List를 Command Queue에 적용 시킨다.
    ThrowIfFailed(m_CommandList->Close());
    ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// GPU의 작업이 끝날때 까지 기다린다.
	FlushCommandQueue();

	// ==================   #9   ==================
	// Viewport 설정
	// ============================================

	// 그리고 Viewport와 scissorRect 값을 갱신해준다.
	m_ScreenViewport.TopLeftX = 0;
	m_ScreenViewport.TopLeftY = 0;
	m_ScreenViewport.Width    = static_cast<float>(m_ClientWidth);
	m_ScreenViewport.Height   = static_cast<float>(m_ClientHeight);
	m_ScreenViewport.MinDepth = 0.0f;
	m_ScreenViewport.MaxDepth = 1.0f;

    m_ScissorRect = { 0, 0, m_ClientWidth, m_ClientHeight};
}
 
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// WM_ACTIVATE 는 윈도우가 Active Inactive 되었을 때 호출 된다.
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			m_AppPaused = true;
			m_Timer.Stop();
		}
		else
		{
			m_AppPaused = false;
			m_Timer.Start();
		}
		return 0;

	// WM_SIZE 윈도우가 리사이징 될 때 호출 된다. 
	case WM_SIZE:
		// 새로운 클라이언트 영역을 갱신한다.
		m_ClientWidth  = LOWORD(lParam);
		m_ClientHeight = HIWORD(lParam);
		if( m_d3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				m_AppPaused = true;
				m_Minimized = true;
				m_Maximized = false;
			}	 
			else if( wParam == SIZE_MAXIMIZED )
			{
				m_AppPaused = false;
				m_Minimized = false;
				m_Maximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// 최소화 에서 돌아 왔을 때
				if( m_Minimized )
				{
					m_AppPaused = false;
					m_Minimized = false;
					OnResize();
				}
				// 최대화 에서 돌아왔을 때
				else if( m_Maximized )
				{
					m_AppPaused = false;
					m_Maximized = false;
					OnResize();
				}
				else if( m_Resizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE 가생이를 잡으면 발생한다.
	case WM_ENTERSIZEMOVE:
		m_AppPaused = true;
		m_Resizing  = true;
		m_Timer.Stop();
		return 0;

	// WM_EXITSIZEMOVE 가생이를 놓으면 발생한다.
	case WM_EXITSIZEMOVE:
		m_AppPaused = false;
		m_Resizing  = false;
		m_Timer.Start();
		OnResize(); // 여기서 리사이즈를 한다.
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// 너무 작아지지 않게 한다.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            Set4xMsaaState(!m_4xMsaaState);

        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_hAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWnd";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// 사용자 영역 계산.
	RECT R = { 0, 0, m_ClientWidth, m_ClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

	m_hMainWnd = CreateWindow(L"MainWnd", m_MainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hAppInst, 0);
	if( !m_hMainWnd )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(m_hMainWnd, SW_SHOW);
	UpdateWindow(m_hMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// d3d12 디버그 레이어 활성화 (*** 이게 세상에서 제일 중요 ㄹㅇㅋㅋ ***)
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif
	// ==================   #1   ==================
	// 하드웨어 어뎁터를 대표하는 ID3D12Device 초기화
	// ============================================

	// DXGI 개체를 생성하는 팩토리 생성
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // 디폴트 어뎁터
		D3D_FEATURE_LEVEL_11_0, // 최소로 지원하는게 D3D11
		IID_PPV_ARGS(&m_d3dDevice)); // 출력변수

	// 만약 그래픽 카드가 없다면, Warp 어댑터를 만든다.
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_d3dDevice)));
	}

	// ==================   #2   ==================
	// CPU, GPU 동기화를 위한 Fence와 View(Descriptor) Size
	// ============================================

	// fence 객체를 만들고
	ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_Fence)));

	// GPU 마다 정해진 view 크기 값 가져오기
	m_RtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ==================   #3   ==================
	// Multi-Sampling Anti-Aliasing 수준 설정하기
	// ============================================

	// d3d11 급에서는 4X MSAA를 지원한다.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};

	msQualityLevels.Format = m_BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(m_d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
	
#ifdef _DEBUG
    LogAdapters();
#endif

	CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3DApp::CreateCommandObjects()
{

	// ==================   #4   ==================
	// Command Queue, Command Allocator, Command List 생성하기
	// ============================================

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// DIRECT 타입으로 큐를 먼저 만들고
	ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
	// 그에 맞춰서 얼로케이터를 만든 다음에
	ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_CommandAllocator.GetAddressOf())));
	// 얼로케이터를 넘겨주면서 리스트를 만든다.
	ThrowIfFailed(m_d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_CommandAllocator.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(m_CommandList.GetAddressOf())));

	// 닫힌 상태로 시작
	// 명령 목록 참조를 위한 Reset을 하려면 닫힌 상태로 해야 한다.
	m_CommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// ==================   #5   ==================
	// Swap Chain 설정과 생성
	// ============================================

	// 이전에 있던 ComPtr 해제
    m_SwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = m_ClientWidth;
    sd.BufferDesc.Height = m_ClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = m_BackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = m_hMainWnd;
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// SwapChain은 Command Queue를 이용해서 자동으로 flush를 수행한다고 한다.
    ThrowIfFailed(m_dxgiFactory->CreateSwapChain(
		m_CommandQueue.Get(),
		&sd, 
		m_SwapChain.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
    m_CurrentFence++;

    // Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
    ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
    if(m_Fence->GetCompletedValue() < m_CurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.  
        ThrowIfFailed(m_Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));

        // Wait until the GPU hits current fence event is fired.
		if (eventHandle != NULL)
		{
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}

ID3D12Resource* D3DApp::GetCurrentBackBuffer()const
{
	return m_SwapChainBuffer[m_CurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_CurrBackBuffer,
		m_RtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetDepthStencilView()const
{
	return m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
    
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if( (m_Timer.GetTotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = m_MainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(m_hMainWnd, windowText.c_str());
		
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while(m_dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        //OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        //OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, m_BackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        //::OutputDebugString(text.c_str());
    }
}