
#include "mainheader.h"
#include <WindowsX.h>
#include "GameTimer.h"
#include <comdef.h>

// 윈도우 전역 변수
HINSTANCE   g_hInst;       
HWND        g_hWnd;
WCHAR szTitle[] = L"InitD3D12";                  
WCHAR szWindowClass[] = L"MainWnd";            

int g_ClientWidth = 800;
int g_ClientHeight = 600;

GameTimer* g_timer = nullptr;
bool g_bAppPaused = false;

// D3D 전역 변수
Microsoft::WRL::ComPtr<IDXGIFactory4> g_dxgiFactory;
Microsoft::WRL::ComPtr<IDXGISwapChain> g_SwapChain;
Microsoft::WRL::ComPtr<ID3D12Device> g_d3dDevice;

Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence;
UINT64 g_CurrentFence = 0;

Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;

static const int SwapChainBufferCount = 2;
int g_CurrBackBuffer = 0;
Microsoft::WRL::ComPtr<ID3D12Resource> g_SwapChainBuffer[SwapChainBufferCount];
Microsoft::WRL::ComPtr<ID3D12Resource> g_DepthStencilBuffer;

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_RtvHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_DsvHeap;

D3D12_VIEWPORT g_ScreenViewport;
D3D12_RECT g_ScissorRect;

UINT g_RenderTargetDescSize = 0;
UINT g_DepthStencilDescSize = 0;
UINT g_ConstantBufferDescSize = 0;

bool      g_b4xMsaaState = false;    // 4X MSAA enabled
UINT      g_4xMsaaQuality = 0;      // quality level of 4X MSAA

DXGI_FORMAT g_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_FORMAT g_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

// Init 함수
bool InitMainWindow(HINSTANCE _hInstance);
bool InitDirect3D();

// Log 함수
void LogAdapters();
void LogAdapterOutputs(IDXGIAdapter* adapter);
void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

// 도우미 함수
ID3D12Resource* GetCurrentBackBuffer();
D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView();
D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView();

// Loop 함수
void CalculateFrameStats();
// void Update(const GameTimer* const _timer);
void Draw(const GameTimer* const _timer);

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    g_hInst = hInstance;

    // 윈도우 초기화
    if (!InitMainWindow(hInstance))
    {
        return FALSE;
    }
    
    // D3D 초기화
    if (!InitDirect3D())
    {
        return FALSE;
    }

    MSG msg = { 0 };

    // 게임 타이머 리셋
    g_timer = new GameTimer;
    g_timer->Reset();

    // 기본 메시지 루프입니다:
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_timer->Tick();
            if (!g_bAppPaused)
            {
                CalculateFrameStats();
                // Update()
                Draw(g_timer);
            }
            else
            {
                Sleep(100);
            }
            // Update
            // Render
        }
    }

    return (int) msg.wParam;
}

bool InitMainWindow(HINSTANCE _hInstance)
{
    WNDCLASS wcex;
    
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = _hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;

    if (!RegisterClass(&wcex))
    {
        MessageBox(0, L"RegisterClass Failed.", 0, 0);
        return false;
    }

    g_hInst = _hInstance; 

    RECT R = { 0, 0, g_ClientWidth, g_ClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    g_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, g_hInst, nullptr);

    if (!g_hWnd)
    {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        return false;
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    return true;
}

bool InitDirect3D()
{ 
    HRESULT hr;

    // d3d12 디버그 레이어 활성화 (*** 이게 세상에서 제일 중요 ㄹㅇㅋㅋ ***)
#if defined(DEBUG) || defined(_DEBUG)

    ComPtr<ID3D12Debug> debugController;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    //hr = D3D12GetDebugInterface(__uuidof(**(&debugController)), IID_PPV_ARGS_Helper(&debugController));
    if (FAILED(hr))
    {
        MessageBox(0, L"D3D12DebugInterface Failed.", 0, 0);
        return false;
    }        
    debugController->EnableDebugLayer(); 
#endif
    // ==================   #1   ==================
    // 하드웨어 어뎁터를 대표하는 ID3D12Device 초기화
    // ============================================
    hr = D3D12CreateDevice(
        nullptr, // 디폴트 어뎁터
        D3D_FEATURE_LEVEL_11_0, // 최소로 지원하는게 D3D11
        IID_PPV_ARGS(&g_d3dDevice) // 출력변수
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"D3D12CreateDevice Failed.", 0, 0);
        return false;
    }

    // DXGI 개체를 생성하는 팩토리 생성
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory));
    // hr = CreateDXGIFactory1(__uuidof(**(&g_dxgiFactory)), IID_PPV_ARGS_Helper(&g_dxgiFactory));

    if (FAILED(hr))
    {
        MessageBox(0, L"CreateDXGIFactory1 Failed.", 0, 0);
        return false;
    }

    // 만약 그래픽 카드가 없다면, Warp 어댑터를 만든다.
    if (FAILED(hr))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        hr = g_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));

        if (FAILED(hr))
        {
            MessageBox(0, L"g_dxgiFactory->EnumWarpAdapter Failed.", 0, 0);
            return false;
        }

        hr = D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&g_d3dDevice)
        );

        if (FAILED(hr))
        {
            MessageBox(0, L"D3D12CreateWarpDevice Failed.", 0, 0);
            return false;
        }
    }

    // ==================   #2   ==================
    // CPU, GPU 동기화를 위한 Fence와 View(Descriptor) Size
    // ============================================

    // fence 객체를 만들고
    hr = g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence));
    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CreateFence Failed.", 0, 0);
        return false;
    }

    // GPU 마다 정해진 view 크기 값 가져오기
    g_RenderTargetDescSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    g_DepthStencilDescSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    g_ConstantBufferDescSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // ==================   #3   ==================
    // Multi-Sampling Anti-Aliasing 수준 설정하기
    // ============================================

    // d3d11 급에서는 4X MSAA를 지원한다.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;

    msQualityLevels.Format = g_BackBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    
    hr = g_d3dDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CheckFeatureSupport Failed.", 0, 0);
        return false;
    }
    g_4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(g_4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
    LogAdapters();
#endif

    // ==================   #4   ==================
    // Command Queue, Command Allocator, Command List 생성하기
    // ============================================

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    // DIRECT 타입으로 큐를 먼저 만들고
    hr = g_d3dDevice->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(g_CommandQueue.GetAddressOf())
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CreateCommandQueue Failed.", 0, 0);
        return false;
    }
    // 그에 맞춰서 얼로케이터를 만든 다음에
    hr = g_d3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(g_CommandAllocator.GetAddressOf())
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CreateCommandAllocator Failed.", 0, 0);
        return false;
    }
    // 얼로케이터를 넘겨주면서 리스트를 만든다.
    hr = g_d3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        g_CommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(g_CommandList.GetAddressOf())
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CreateCommandList Failed.", 0, 0);
        return false;
    }
    // 닫힌 상태로 시작
    // 명령 목록 참조를 위한 Reset을 하려면 닫힌 상태로 해야 한다.
    g_CommandList->Close();

    // ==================   #5   ==================
    // Swap Chain 설정과 생성
    // ============================================

    g_SwapChain.Reset(); // ComPtr 해제

    DXGI_SWAP_CHAIN_DESC sd;

    DXGI_MODE_DESC bfd;
    bfd.Width = g_ClientWidth;
    bfd.Height = g_ClientHeight;
    bfd.RefreshRate.Numerator = 60;
    bfd.RefreshRate.Denominator = 1;
    bfd.Format = g_BackBufferFormat;
    bfd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bfd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    sd.BufferDesc = bfd;

    DXGI_SAMPLE_DESC spd;
    spd.Count = g_b4xMsaaState ? 4 : 1;
    spd.Quality = g_b4xMsaaState ? (g_4xMsaaQuality - 1) : 0;

    sd.SampleDesc = spd;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = g_hWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // SwapChain은 Command Queue를 이용해서 자동으로 flush를 수행한다고 한다.
    hr = g_dxgiFactory->CreateSwapChain(
        g_CommandQueue.Get(),
        &sd,
        g_SwapChain.GetAddressOf()
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_dxgiFactory->CreateSwapChain Failed.", 0, 0);
        return false;
    }

    // ==================   #6   ==================
    // Descriptor(View) Heap 설정과 생성
    // ============================================

    // 렌더타겟 힙
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    
    hr = g_d3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc,
        IID_PPV_ARGS(&g_RtvHeap)
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CreateDescriptorHeap. - RTV", 0, 0);
        return false;
    }

    // 뎁스 스텐실 힙
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;

    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    hr = g_d3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc,
        IID_PPV_ARGS(&g_DsvHeap)
    );

    if (FAILED(hr))
    {
        MessageBox(0, L"g_d3dDevice->CreateDescriptorHeap. - DSV", 0, 0);
        return false;
    }

    // ======================================
    //7번 8번 친구들은 CommandList로 해줘야한다.
    // ======================================

    g_CommandList->Reset(g_CommandAllocator.Get(), nullptr);

    // 일단 냅다 reset을 해버리고
    for (int i = 0; i < SwapChainBufferCount; i++)
    {
        g_SwapChainBuffer[i].Reset();
    }
    g_DepthStencilBuffer.Reset();


    // ==================   #7   ==================
    // RenderTarget View (Descriptor) 설정과 생성
    // ============================================

    // 도우미 CPU Descriptor 구조체를 0번으로 초기화 한다.
    // 후면 버퍼는 CPU 자원이고, 이것을 View로 만들어서 GPU에게 넘겨줘야 하는 것

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(
        g_RtvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        // 스왑체인에서 버퍼를 얻고
        hr = g_SwapChain->GetBuffer(
            i, IID_PPV_ARGS(&g_SwapChainBuffer[i])
        );

        if (FAILED(hr))
        {
            MessageBox(0, L"g_SwapChain->GetBuffer", 0, 0);
            return false;
        }

        // 해당버퍼에 대한 render target view를 만들고
        g_d3dDevice->CreateRenderTargetView(
            g_SwapChainBuffer[i].Get(),
            nullptr, // 일단은 nullpr 이다.
            rtvHeapHandle
        );

        // 힙 핸들을 다음 번호로 넘긴다.
        rtvHeapHandle.Offset(1, g_RenderTargetDescSize);
    }

    // ==================   #8   ==================
    // Depth-Stencil View (Descriptor) 설정과 생성
    // ============================================

    // 얘는 GPU 자원들이여서, GPU 힙에 존재한다.
    // 그래서 함수 동작도 CPU 자원들과 다르다.
    D3D12_RESOURCE_DESC depthStencilDesc;
    // 일단은 깊이 정보만 저장한다.
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = g_ClientWidth;
    depthStencilDesc.Height = g_ClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    // 일단은 여기서는 TypeLess 이다.
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    // DXGI_SAMPLE_DESC
    spd = { 0 };
    spd.Count = g_b4xMsaaState ? 4 : 1;
    spd.Quality = g_b4xMsaaState ? (g_4xMsaaQuality - 1) : 0;
    depthStencilDesc.SampleDesc = spd;

    // UKnown + Depth Stencil로 넘겨준다.
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // 자원 지우기 설정을 나타내는 구조체 CLAER_VALUE
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = g_DepthStencilFormat;
    optClear.DepthStencil.Depth = 1.f;
    optClear.DepthStencil.Stencil = 0;

    // 일단 생성하는 것은 Default 값으로 만들어 준다.
    CD3DX12_HEAP_PROPERTIES cHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    // 이게 GPU 자원 전용으로 Resource를 생성하는 함수이다.
    hr = g_d3dDevice->CreateCommittedResource(
        &cHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(g_DepthStencilBuffer.GetAddressOf())
    );

    // Resource으로 View를 만든다.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = g_DepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0; // 일단 mipmap은 없다.
    // View Desc 구조체를 채워주고

    g_d3dDevice->CreateDepthStencilView(
        g_DepthStencilBuffer.Get(),
        &dsvDesc, 
        GetDepthStencilView() // g_DsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    // 이제 GPU에 넘겨줄 수 있는 상태(Resource States)로 만든다.
    // ResourceBarrier 구조체로 관리를 해준다.
 
    // D3D12_RESOURCE_STATE_DEPTH_WRITE 상태
    D3D12_RESOURCE_BARRIER DepthBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        g_DepthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    // 커맨드 리스트에서 리소스 베리어를 관리한다.
    g_CommandList->ResourceBarrier(
        1, 
        &DepthBarrierTransition // Barrier 구조체
    );

    // ==================   #9   ==================
    // Viewport 설정
    // ============================================

    // 요것도 구조체가 있다.
    // D3D12_VIEWPORT
    g_ScreenViewport.TopLeftX = 0.f;
    g_ScreenViewport.TopLeftY = 0.f;
    g_ScreenViewport.Width = static_cast<float>(g_ClientWidth);
    g_ScreenViewport.Height = static_cast<float>(g_ClientHeight);
    g_ScreenViewport.MinDepth = 0.f;
    g_ScreenViewport.MaxDepth = 1.f;

    // 커맨드 리스트에서 viewport를 관리해준다.
    // 더 멋있는 거 하려면 뷰포트 여러개 만들 수 도 있다.

    // ==================   #11   =================
    // Scissor Rectangle 설정
    // ============================================

    // culling을 하는 최적화 설정이다.
    g_ScissorRect = { 0, 0, g_ClientWidth, g_ClientHeight };
    // 얘도 커맨드 리스트에서 관리한다.

    // ==================   #12   ==================
    // Command List를 닫고, Queue에 올려준다.
    // ============================================

    g_CommandList->Close();
    ID3D12CommandList* commandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // (임시) GPU의 작업이 끝날때 까지 기다린다.
    // fence의 값을 하나 늘린다.
    g_CurrentFence++;
    hr = g_CommandQueue->Signal(g_Fence.Get(), g_CurrentFence);

    // GPU가 CPU의 fence 번호에 도달할 때 까지 기다린다.
    if (g_Fence->GetCompletedValue() < g_CurrentFence)
    {
        // GPU가 작업을 끝냈을 때 이벤트를 정의한다.
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        // 그 이벤트를 등록해주고
        hr = g_Fence->SetEventOnCompletion(g_CurrentFence, eventHandle);
        // 이벤트가 발생하기 전 까지 INFINITE를 기다린다.
        DWORD ret = WaitForSingleObject(eventHandle, INFINITE);
        // 이벤트를 해제한다.
        CloseHandle(eventHandle);
    }

    return true;
}

ID3D12Resource* GetCurrentBackBuffer()
{
    return g_SwapChainBuffer[g_CurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView()
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        g_RtvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_CurrBackBuffer,
        g_RenderTargetDescSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView()
{
    return g_DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void CalculateFrameStats()
{
    // 일단 창 제목에 출력한다.
    static int frameCount = 0;
    static float timeElapsed = 0.f;

    if (g_timer == nullptr)
    {
        return;
    }
    frameCount++;

    if ((g_timer->GetTotalTime() - timeElapsed) >= 1.f)
    {
        float fps = (float)frameCount;
        float msPerFrame = 1000.f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring msPerFrameStr = to_wstring(msPerFrame);

        wstring windowText = wstring(szTitle) + L" fps : " + fpsStr + L" msPerFrame : " + msPerFrameStr;

        SetWindowText(g_hWnd, windowText.c_str());

        frameCount = 0;
        timeElapsed += 1.f;
    }
}

void Draw(const GameTimer* const _timer)
{
    HRESULT hr;
    // Command Allocator를 재설정 한다.
    hr = g_CommandAllocator->Reset();

    // Command List를 재설정한다.
    // 초기 상태는 nullptr 로 한다.
    hr = g_CommandList->Reset(g_CommandAllocator.Get(), nullptr);

    // 그림을 그릴 back buffer의 Barrier를 D3D12_RESOURCE_STATE_RENDER_TARGET으로 바꾼다.
    D3D12_RESOURCE_BARRIER bufferBarrier_RENDER_TARGET = CD3DX12_RESOURCE_BARRIER::Transition(
        GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    g_CommandList->ResourceBarrier(
        1,
        &bufferBarrier_RENDER_TARGET
    );

    // Viewport와 ScissorRects 를 설정한다.
    g_CommandList->RSSetViewports(1, &g_ScreenViewport);
    g_CommandList->RSSetScissorRects(1, &g_ScissorRect);

    // back buffer를 초기화한다.
    g_CommandList->ClearRenderTargetView(
        GetCurrentBackBufferView(),
        Colors::LightSteelBlue,
        0,
        nullptr
    );

    // depth buffer를 초기화한다
    g_CommandList->ClearDepthStencilView(
        GetDepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.f,
        0,
        0,
        nullptr
    );

    // 렌더 타겟들을 최종으로 그림을 그릴 Output-Merge로 설정한다.
    D3D12_CPU_DESCRIPTOR_HANDLE BackBufferHandle = GetCurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = GetDepthStencilView();
    g_CommandList->OMSetRenderTargets(1, &BackBufferHandle, true, &DepthStencilHandle);

    // 그림을 그릴 back buffer의 Barrier를 D3D12_RESOURCE_STATE_PRESENT으로 바꾼다.
    D3D12_RESOURCE_BARRIER bufferBarrier_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(
        GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    g_CommandList->ResourceBarrier(
        1,
        &bufferBarrier_PRESENT
    );

    // Command List 기록을 끝낸다.
    hr = g_CommandList->Close();

    // Command List를 Command Queue에 추가한다.
    ID3D12CommandList* commandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // front와 back 버퍼를 교환한다.
    hr = g_SwapChain->Present(0, 0);
    g_CurrBackBuffer = (g_CurrBackBuffer + 1) % SwapChainBufferCount;

    // (임시) GPU의 작업이 끝날때 까지 기다린다.
    // fence의 값을 하나 늘린다.
    g_CurrentFence++;
    hr = g_CommandQueue->Signal(g_Fence.Get(), g_CurrentFence);

    // GPU가 CPU의 fence 번호에 도달할 때 까지 기다린다.
    if (g_Fence->GetCompletedValue() < g_CurrentFence)
    {
        // GPU가 작업을 끝냈을 때 이벤트를 정의한다.
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        // 그 이벤트를 등록해주고
        hr = g_Fence->SetEventOnCompletion(g_CurrentFence, eventHandle);
        // 이벤트가 발생하기 전 까지 INFINITE를 기다린다.
        DWORD ret = WaitForSingleObject(eventHandle, INFINITE);
        // 이벤트를 해제한다.
        CloseHandle(eventHandle);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            g_bAppPaused = true;
            if (g_timer)
            {
                g_timer->Stop();
            }
        }
        else
        {
            g_bAppPaused = false;
            if (g_timer)
            {
                g_timer->Start();
            }
        }
        break;
    }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while (g_dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);

        ++i;
    }

    for (size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        adapterList[i]->Release();
        adapterList[i] = nullptr;
    }
}

void LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);

        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, g_BackBufferFormat);

        output->Release();
        output = nullptr;

        ++i;
    }
}

void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for (auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}
