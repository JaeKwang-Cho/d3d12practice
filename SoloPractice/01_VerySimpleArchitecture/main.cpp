// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "D3D12Renderer.h"

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

D3D12Renderer* g_pRenderer = nullptr;

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


    MSG msg = {};

    // 기본으로 PeekMessage를 사용
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Rendering
        }
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