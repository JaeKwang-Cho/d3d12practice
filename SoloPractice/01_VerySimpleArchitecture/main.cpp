// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "D3D12Renderer.h"

// D3D 라이브러리 링크
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// D3D12 Agility 설정
// 방법 : https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#OS
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

#if defined(_M_AMD64)
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\x64\\"; }
#endif

// 윈도우 전역 변수:
HINSTANCE g_hInst;    
HWND g_hWnd;
WCHAR g_szTitle[] = L"VerySimpleArchitecture";
WCHAR g_szWindowClass[] = L"Main Window";
int g_ClientWidth = 1280;
int g_ClientHeight = 720;

// D3D12 전역변수
D3D12_HEAP_PROPERTIES HEAP_PROPS_DEFAULT = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
D3D12_HEAP_PROPERTIES HEAP_PROPS_UPLOAD = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

// 렌더링 전역 변수
D3D12Renderer* g_pRenderer = nullptr;
void* g_pMeshObj = nullptr;

void* g_pTexHandle0 = nullptr;
void* g_pTexHandle1 = nullptr;

// test
float g_fOffsetX = 0.f;
float g_fOffsetY = 0.f;
float g_fSpeed = 0.01f;
 
// 렌더링 함수
void RunGame();
void Update();

// 윈도우 프로시져
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 초기화 함수
bool InitWindow(HINSTANCE _hInstance);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // cpp runtime library에서 제공하는 debug flag를 켜는 것이다.
    // Memory Heap corruption 혹은 Memory Leak이 있는지 확인한다.
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // 윈도우 초기화
    g_hInst = hInstance;
    if (!InitWindow(hInstance))
    {
        // 문제 생기면 무조건 크래쉬 시키는 코드이다.
        __debugbreak();
        return FALSE;
    }

    // D3D 초기화
    g_pRenderer = new D3D12Renderer;
    g_pRenderer->Initialize(g_hWnd, true, true);
    // 간단한 메쉬 만들기
    g_pMeshObj = g_pRenderer->CreateBasicMeshObject_Return_New();
    // 간단한 텍스쳐 만들기
    g_pTexHandle0 = g_pRenderer->CreateTileTexture(16, 16, 192, 128, 255);
    g_pTexHandle1 = g_pRenderer->CreateTileTexture(32, 32, 128, 255, 192);

    MSG msg = {};

    // 기본으로 PeekMessage를 사용
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Rendering
            RunGame();
        }
    }

    if (g_pMeshObj) {
        g_pRenderer->DeleteBasicMeshObject(g_pMeshObj);
        g_pMeshObj = nullptr;
    }
    if (g_pTexHandle0)
    {
        g_pRenderer->DeleteTexture(g_pTexHandle0);
        g_pTexHandle0 = nullptr;
    }
    if (g_pTexHandle1)
    {
        g_pRenderer->DeleteTexture(g_pTexHandle1);
        g_pTexHandle1 = nullptr;
    }
    if (g_pRenderer) {
        delete g_pRenderer;
        g_pRenderer = nullptr;

        // resource leak!!!
        IDXGIDebug1* pDebug = nullptr;
        // 해제하지 않은 Resource 들에 대한 정보를 출력하게 한다.
        // 만약 해제하지 않은 Resource를 참조하는 Resource가 많다면,
        // 그 Resource들도 전부 해제가 안되어서 엄청 많이 뜰 수도 있다.
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
        {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
            pDebug->Release();
        }
    }
#ifdef _DEBUG
    // 여기서 걸리면 위에서 지정해놓은 디버기 플래그에 해당하는 문제가 터진 것이다.
    // 디버그 힙에서 콜스택을 추적해준다.
    _ASSERT(_CrtCheckMemory());
#endif

    return (int) msg.wParam;
}

void RunGame()
{
    // begin
    g_pRenderer->BeginRender();

    // game business logic
    Update();

    // rendering objects
    // 하나의 object에 대해서 2번 렌더링을 다르게 한다.
    g_pRenderer->RenderMeshObject(g_pMeshObj, g_fOffsetX, 0.f, g_pTexHandle0);
    g_pRenderer->RenderMeshObject(g_pMeshObj, 0.f, g_fOffsetY, g_pTexHandle1);
    //g_pRenderer->RenderMeshObject(g_pMeshObj, -g_fOffsetX, 0.f);
    //g_pRenderer->RenderMeshObject(g_pMeshObj, 0.f, -g_fOffsetY);
    //g_pRenderer->RenderMeshObject(g_pMeshObj, g_fOffsetX, g_fOffsetY);
    //g_pRenderer->RenderMeshObject(g_pMeshObj, -g_fOffsetX, -g_fOffsetY);
    //g_pRenderer->RenderMeshObject(g_pMeshObj, -g_fOffsetX, g_fOffsetY);
    //g_pRenderer->RenderMeshObject(g_pMeshObj, g_fOffsetX, -g_fOffsetY);
    // end
    g_pRenderer->EndRender();

    // Present
    g_pRenderer->Present();
}

void Update()
{
    bool bDirChange = false;
    g_fOffsetX += g_fSpeed;
    if (g_fOffsetX > 0.75f)
    {
        g_fSpeed *= -1.0f;
    }
    if (g_fOffsetX < -0.75f)
    {
        g_fSpeed *= -1.0f;
    }
    g_fOffsetY += g_fSpeed;
    if (g_fOffsetY > 0.75f)
    {
        g_fSpeed *= -1.0f;
        bDirChange = true;
    }
    if (g_fOffsetY < -0.75f)
    {
        g_fSpeed *= -1.0f;
        bDirChange = true;
    }
    if (bDirChange) {
        void* pTemp = g_pTexHandle0;
        g_pTexHandle0 = g_pTexHandle1;
        g_pTexHandle1 = pTemp;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE) {

        }else{

        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE: 
    {
        if (g_pRenderer) {
            RECT rect;
            GetClientRect(hWnd, &rect);
            DWORD dwWidth = rect.right - rect.left;
            DWORD dwHeight = rect.bottom - rect.top;
            g_pRenderer->UpdateWindowSize(dwWidth, dwHeight);
        }
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

bool InitWindow(HINSTANCE _hInstance)
{
    // WNDCLASS 구조체 채우기
    WNDCLASS wcex = {};

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = _hInstance;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = g_szWindowClass;
    
    // WNDCLASS 등록하기
    if (!RegisterClass(&wcex)) {
        MessageBox(0, L"RegisterClass Failed", 0, 0);
        __debugbreak();
        return false;
    }

    g_hInst = _hInstance;

    // 윈도우 크기 지정
    RECT rect = { 0, 0, g_ClientWidth, g_ClientHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // 윈도우 생성
    g_hWnd = CreateWindow(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, g_hInst, nullptr);
    if (!g_hWnd) {
        MessageBox(0, L"CreateWindow Failed.", 0, 0);
        __debugbreak();
        return false;
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);    

    return true;
}
