// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include <windowsx.h>
#include <filesystem>

// COM
#include <combaseapi.h>

// Game
#include "Game.h"

// 윈도우 전역 변수:
HINSTANCE g_hInst;
HWND g_hWnd;
WCHAR g_szTitle[] = L"App";
WCHAR g_szWindowClass[] = L"Main Window";
int g_ClientWidth = 1280;
int g_ClientHeight = 720;
WCHAR g_tempPath[MAX_PATH];

// Game 전역변수
Game* g_pGame = nullptr;

// 윈도우 프로시져
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 초기화 함수
bool InitWindow(HINSTANCE _hInstance);

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }

#if defined(_M_AMD64)
extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\x64\\"; } // c++20 이므로 이렇게 해줘야 한다.
#endif

static std::filesystem::path GetExeDirectory()
{
    WCHAR wchModuleFileName[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, wchModuleFileName, MAX_PATH);
    return std::filesystem::path(wchModuleFileName).parent_path();
}

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

    // COM 초기화
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        __debugbreak();
    }


    MSG msg = {};

    g_pGame = new Game;
    g_pGame->Initialize_Game(g_hWnd);

    // 기본으로 PeekMessage를 사용
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            g_pGame->Run();
        }
    }

    if (g_pGame)
    {
        delete g_pGame;
        g_pGame = nullptr;
    }

   
#ifdef _DEBUG
    // 여기서 걸리면 위에서 지정해놓은 디버기 플래그에 해당하는 문제가 터진 것이다.
    // 디버그 힙에서 콜스택을 추적해준다.
    _ASSERT(_CrtCheckMemory());
#endif

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE) {

        }
        else {

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
        if (g_pGame) {
            RECT rect;
            GetClientRect(hWnd, &rect);
            DWORD dwWidth = rect.right - rect.left;
            DWORD dwHeight = rect.bottom - rect.top;
            g_pGame->UpdateWindowSize(dwWidth, dwHeight);
        }
    }
    break;
    case WM_LBUTTONDOWN:
        break;
    case WM_MBUTTONDOWN:
        break;
    case WM_RBUTTONDOWN:
    {
        if (g_pGame) {
            g_pGame->OnRButtonDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
    }
    return 0;
    case WM_LBUTTONUP:
        break;
    case WM_MBUTTONUP:
        break;
    case WM_RBUTTONUP:
    {
        if (g_pGame) {
            g_pGame->OnRButtonUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
    }
    return 0;
    case WM_MOUSEMOVE:
    {
        if (g_pGame) {
            g_pGame->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
    }
    return 0;
    case WM_DROPFILES:
    {
    }
    return 0;
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
    // 드래그앤 드롭이 가능하도록 만든다.
    DWORD dwExStyle = WS_EX_ACCEPTFILES;
    // 혹시나 Client와 맞추기 위해, 크기를 바꾸고, 최대화를 시키지 못하게 만드는 플래그다.
    // DWORD dwStyleNoResizable = WS_OVERLAPPEDWINDOW & (~WS_MAXIMIZEBOX) & (~WS_THICKFRAME);
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
