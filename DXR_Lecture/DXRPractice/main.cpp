// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include <windowsx.h>
#include <filesystem>
#include "Game.h"

// D3D 라이브러리 링크
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib") 

// D3D12 Agility 설정
// 방법 : https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#OS
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }

#if defined(_M_AMD64)
extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\"; } 
#endif

// 윈도우 전역 변수:
HINSTANCE g_hInst;
HWND g_hWnd;
WCHAR g_szTitle[] = L"DXR Practices";
WCHAR g_szWindowClass[] = L"Main Window";
int g_ClientWidth = 1280;
int g_ClientHeight = 720;

Game* g_pGame = nullptr;

// 윈도우 프로시져
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 초기화 함수
bool InitWindow(HINSTANCE _hInstance);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // C++ 런타임 라이브러리의 메모리 누수 검사 필터 설정
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // 윈도우 초기화
    g_hInst = hInstance;
    if (!InitWindow(hInstance))
    {
        __debugbreak();
        return FALSE;
    }

    // COM 초기화
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        __debugbreak();
    }

    MSG msg = {};

    g_pGame = new Game;
    if (!g_pGame->Initialize(g_hWnd, true, true, false))
    {
        __debugbreak();
        return FALSE;
    }

    // 게임 루프 (PeekMessage 기반 메인 루프)
    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            if (g_pGame)
            {
                g_pGame->Run();
            }
        }
    }

    // 게임 해제
    if (g_pGame)
    {
        delete g_pGame;
        g_pGame = nullptr;
    }

    // 메모리 누수 리포트 출력
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }

#ifdef _DEBUG
    _ASSERT(_CrtCheckMemory());
#endif

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const int iMouseX = GET_X_LPARAM(lParam);
    const int iMouseY = GET_Y_LPARAM(lParam);

    switch (message)
    {
    case WM_SIZE:
    {
        if (g_pGame)
        {
            RECT rect;
            GetClientRect(hWnd, &rect);
            DWORD dwWndWidth = rect.right - rect.left;
            DWORD dwWndHeight = rect.bottom - rect.top;
            g_pGame->UpdateWindowSize(dwWndWidth, dwWndHeight);
        }
    }
    break;
    case WM_MOUSEMOVE:
        if (g_pGame)
            g_pGame->OnMouseMove(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_LBUTTONDOWN:
        if (g_pGame)
            g_pGame->OnMouseLButtonDown(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_LBUTTONUP:
        if (g_pGame)
            g_pGame->OnMouseLButtonUp(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_RBUTTONDOWN:
        if (g_pGame)
            g_pGame->OnMouseRButtonDown(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_RBUTTONUP:
        if (g_pGame)
            g_pGame->OnMouseRButtonUp(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_MBUTTONDOWN:
        if (g_pGame)
            g_pGame->OnMouseMButtonDown(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_MBUTTONUP:
        if (g_pGame)
            g_pGame->OnMouseMButtonUp(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_KEYDOWN:
    {
        if (g_pGame)
        {
            UINT uiScanCode = (0x00ff0000 & lParam) >> 16;
            UINT vkCode = MapVirtualKey(uiScanCode, MAPVK_VSC_TO_VK);
            if (!(lParam & 0x40000000))
            {
                g_pGame->OnKeyDown(vkCode, uiScanCode);
            }
        }
    }
    break;
    case WM_KEYUP:
    {
        if (g_pGame)
        {
            UINT uiScanCode = (0x00ff0000 & lParam) >> 16;
            UINT vkCode = MapVirtualKey(uiScanCode, MAPVK_VSC_TO_VK);
            g_pGame->OnKeyUp(vkCode, uiScanCode);
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
    WNDCLASS wcex = {};

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = _hInstance;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = g_szWindowClass;

    if (!RegisterClass(&wcex)) {
        MessageBox(0, L"RegisterClass Failed", 0, 0);
        __debugbreak();
        return false;
    }

    g_hInst = _hInstance;

    RECT rect = { 0, 0, g_ClientWidth, g_ClientHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    DWORD dwExStyle = WS_EX_ACCEPTFILES;
    g_hWnd = CreateWindowEx(dwExStyle, g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
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
