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
	// �޽��� �������� ����ϴ� ���ν��� �Լ��� ���⼭ �ش�.
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
    // D3DApp�� �ϳ��� �ʱ�ȭ
    assert(m_App == nullptr);
    m_App = this;
}

D3DApp::~D3DApp()
{
	if (m_d3dDevice != nullptr)
	{
		// ���ֱ� ���� Command Queue�� ����
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
	// ȭ�� ��Ⱦ��
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

        // ���ο� ��Ƽ ���ø��̸� ����ü���� �ٽ� ������ �Ѵ�.
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
		// D3DApp Ŭ���� �ȿ��� �޽��� ������ ������.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// ����� ������ �κ�
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

	// View �� �ʱ⿡�� �������� �ʿ䰡 �ִ�.
	OnResize();

	return true;
}
 
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// ==================   #6   ==================
	// Descriptor(View) Heap ������ ����
	// ============================================

	// ����Ÿ�� ��
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, 
		IID_PPV_ARGS(m_RtvHeap.GetAddressOf())
	));

	// ���� ���ٽ� ��
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
	//7�� 8�� ģ������ CommandList�� ������Ѵ�.
	// ======================================

	assert(m_d3dDevice);
	assert(m_SwapChain);
    assert(m_CommandAllocator);

	// GPU�� �Ѿ �ִ� Resource�� �ٽ� �׸��� �ʹٸ�, Flush�� ���ش�.
	FlushCommandQueue();

	// Command List reset�ϰ�
    ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	// ���۵� reset�� �ع�����
	for (int i = 0; i < SwapChainBufferCount; ++i)
	{
		m_SwapChainBuffer[i].Reset();
	}
    m_DepthStencilBuffer.Reset();
	
	// Swap Chain�� ũ�⸦ �ٽ� �������ش�.
    ThrowIfFailed(m_SwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		m_ClientWidth, 
		m_ClientHeight,
		m_BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	m_CurrBackBuffer = 0;


	// ==================   #7   ==================
	// RenderTarget View (Descriptor) ������ ����
	// ============================================
 
	// CD3DX12_CPU_DESCRIPTOR_HANDLE�� �̿��ؼ� �� �� ���ϰ�(?)
	// ����� CPU Descriptor ����ü�� 0������ �ʱ�ȭ �Ѵ�.
	// �ĸ� ���۴� CPU �ڿ��̰�, �̰��� View�� ���� GPU���� �Ѱ���� �ϴ� ��

	// Render target heap�� ���� handle����������
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// heap handle�� Buffer�� view(descriptor)�� �����.
		// ����ü�ο��� ���۸� ���
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffer[i])));
		// �ش���ۿ� ���� render target view�� �����
		m_d3dDevice->CreateRenderTargetView(m_SwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// �� �ڵ��� ���� ��ȣ�� �ѱ��.
		rtvHeapHandle.Offset(1, m_RtvDescriptorSize);
	}

	// ==================   #8   ==================
	// Depth-Stencil View (Descriptor) ������ ����
	// ============================================

	// ��� GPU �ڿ����̿���, GPU ���� �����Ѵ�.
	// �׷��� �Լ� ���۵� CPU �ڿ���� �ٸ���.

    // ��� Depth-Stencil View�� ����� ���̴�.
    D3D12_RESOURCE_DESC depthStencilDesc = {};
	// Render Target�� �Ȱ��� �Ӽ��� ������ְ�
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_ClientWidth;
    depthStencilDesc.Height = m_ClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO é�ʹ� depth buffer���� ���� �������� Shader Resource View Format�� depth buffer���� �䱸�Ѵ�.
	// (SSAO chapter requires an SRV to the depth buffer to read from
	// �׷���, �Ȱ��� resource���� 2���� view�� ������ �Ѵ�.
	// (Therefore, because we need to create two views to the same resource)
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// �׷��� �ϴ� TYPELESS���� depth buffer resource�� ������ �Ѵ�.
	// (we need to create the depth buffer resource with a typeless format.)  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

	// DXGI_SAMPLE_DESC
    depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;

	// UKnown + Depth Stencil�� �Ѱ��ش�.
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// �ڿ� ����� ������ ��Ÿ���� ����ü CLAER_VALUE
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = m_DepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	// �ϴ� �����ϴ� ���� Default ������ ����� �ش�.
	CD3DX12_HEAP_PROPERTIES cHeapProps(D3D12_HEAP_TYPE_DEFAULT);

	// �̰� GPU �ڿ� �������� Resource�� �����ϴ� �Լ��̴�.
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &cHeapProps,
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(m_DepthStencilBuffer.GetAddressOf())));

	// Depth-Stencil View�� ���� ����ü�� ä���ְ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0; // ��� mip level�� 0���� ���ش�.

	// Resource�� ����ü�� View�� �����.
    m_d3dDevice->CreateDepthStencilView(
		m_DepthStencilBuffer.Get(), 
		&dsvDesc, 
		GetDepthStencilView());

	// ���� GPU�� �Ѱ��� �� �ִ� ����(Resource States)�� �����.
	// ResourceBarrier ����ü�� ������ ���ش�.

	// D3D12_RESOURCE_STATE_DEPTH_WRITE ����
	D3D12_RESOURCE_BARRIER DepthBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_DepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE
	);

	// Ŀ�ǵ� ����Ʈ���� ���ҽ� ����� �����Ѵ�.
	m_CommandList->ResourceBarrier(
		1, 
		&DepthBarrierTransition
	);
	
    // ���� Command List�� Command Queue�� ���� ��Ų��.
    ThrowIfFailed(m_CommandList->Close());
    ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// GPU�� �۾��� ������ ���� ��ٸ���.
	FlushCommandQueue();

	// ==================   #9   ==================
	// Viewport ����
	// ============================================

	// �׸��� Viewport�� scissorRect ���� �������ش�.
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
	// WM_ACTIVATE �� �����찡 Active Inactive �Ǿ��� �� ȣ�� �ȴ�.
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

	// WM_SIZE �����찡 ������¡ �� �� ȣ�� �ȴ�. 
	case WM_SIZE:
		// ���ο� Ŭ���̾�Ʈ ������ �����Ѵ�.
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
				
				// �ּ�ȭ ���� ���� ���� ��
				if( m_Minimized )
				{
					m_AppPaused = false;
					m_Minimized = false;
					OnResize();
				}
				// �ִ�ȭ ���� ���ƿ��� ��
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

	// WM_EXITSIZEMOVE �����̸� ������ �߻��Ѵ�.
	case WM_ENTERSIZEMOVE:
		m_AppPaused = true;
		m_Resizing  = true;
		m_Timer.Stop();
		return 0;

	// WM_EXITSIZEMOVE �����̸� ������ �߻��Ѵ�.
	case WM_EXITSIZEMOVE:
		m_AppPaused = false;
		m_Resizing  = false;
		m_Timer.Start();
		OnResize(); // ���⼭ ������� �Ѵ�.
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// �ʹ� �۾����� �ʰ� �Ѵ�.
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

	// ����� ���� ���.
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
	// d3d12 ����� ���̾� Ȱ��ȭ (*** �̰� ���󿡼� ���� �߿� �������� ***)
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif
	// ==================   #1   ==================
	// �ϵ���� ��͸� ��ǥ�ϴ� ID3D12Device �ʱ�ȭ
	// ============================================

	// DXGI ��ü�� �����ϴ� ���丮 ����
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // ����Ʈ ���
		D3D_FEATURE_LEVEL_11_0, // �ּҷ� �����ϴ°� D3D11
		IID_PPV_ARGS(&m_d3dDevice)); // ��º���

	// ���� �׷��� ī�尡 ���ٸ�, Warp ����͸� �����.
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
	// CPU, GPU ����ȭ�� ���� Fence�� View(Descriptor) Size
	// ============================================

	// fence ��ü�� �����
	ThrowIfFailed(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_Fence)));

	// GPU ���� ������ view ũ�� �� ��������
	m_RtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ==================   #3   ==================
	// Multi-Sampling Anti-Aliasing ���� �����ϱ�
	// ============================================

	// d3d11 �޿����� 4X MSAA�� �����Ѵ�.
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
	// Command Queue, Command Allocator, Command List �����ϱ�
	// ============================================

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// DIRECT Ÿ������ ť�� ���� �����
	ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));
	// �׿� ���缭 ��������͸� ���� ������
	ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_CommandAllocator.GetAddressOf())));
	// ��������͸� �Ѱ��ָ鼭 ����Ʈ�� �����.
	ThrowIfFailed(m_d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_CommandAllocator.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(m_CommandList.GetAddressOf())));

	// ���� ���·� ����
	// ��� ��� ������ ���� Reset�� �Ϸ��� ���� ���·� �ؾ� �Ѵ�.
	m_CommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// ==================   #5   ==================
	// Swap Chain ������ ����
	// ============================================

	// ������ �ִ� ComPtr ����
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

	// SwapChain�� Command Queue�� �̿��ؼ� �ڵ����� flush�� �����Ѵٰ� �Ѵ�.
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