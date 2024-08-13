// PixelStreamingClient.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "D3D12Renderer_Client.h"
#include "lz4.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#define MAX_LOADSTRING 100

// D3D12 Agility 설정
// 방법 : https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#OS
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

#if defined(_M_AMD64)
extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\x64\\"; } // c++20 이므로 이렇게 해줘야 한다.
#endif

// 전역 변수
HINSTANCE g_hInst;    
HWND g_hWnd;
WCHAR g_szTitle[] = L"PixelStreamingClient";
WCHAR g_szWindowClass[] = L"Main Window";             
D3D12Renderer_Client* g_pRenderer;

const int g_ClientWidth = 1280;
const int g_ClientHeight = 720;
ULONGLONG g_PixelStreamingTime = 0;

// 테스트용 텍스쳐
UINT8 texturePixels[g_ClientWidth * g_ClientHeight * 4];

// 전역 함수
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitWindow(HINSTANCE _hInstance);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    /*
    const char* input = "1234567890abcdefghijklmnopqrstuvwxyz\n";
    int inputSize = strlen(input) + 1;
    char compressed[100];
    char decompressed[100];

    OutputDebugStringA(input);
    int compressSize = LZ4_compress_fast(input, compressed, inputSize, LZ4_compressBound(inputSize), 100);
    int decompressSize = LZ4_decompress_safe_partial(compressed, decompressed, compressSize, inputSize, 100);
    OutputDebugStringA(decompressed);
    */
    
    // 윈도우 초기화
    g_hInst = hInstance;
    if (!InitWindow(hInstance))
    {
        __debugbreak();
        return FALSE;
    }

    // 렌더러 초기화
    g_pRenderer = new D3D12Renderer_Client;
    if (!g_pRenderer->Initialize(g_hWnd))
    {
        __debugbreak();
        return FALSE;
    }

    // 테스트용 텍스쳐
    for (UINT y = 0; y < g_ClientHeight; y++) {
        for (UINT x = 0; x < g_ClientWidth; x++) {
            texturePixels[(y * g_ClientWidth + x) * 4 + 0] = x % 256;
            texturePixels[(y * g_ClientWidth + x) * 4 + 1] = y % 256;
            texturePixels[(y * g_ClientWidth + x) * 4 + 2] = 0;
            texturePixels[(y * g_ClientWidth + x) * 4 + 3] = 255;
        }
    }

    MSG msg = {0};

    // 기본 메시지 루프입니다:
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Rendering
            ULONGLONG CurrTickTime = GetTickCount64();
            if (CurrTickTime - g_PixelStreamingTime > 16)
            {
                g_PixelStreamingTime = CurrTickTime;
                g_pRenderer->DrawStreamPixels();
            }
        }
    }

    if (g_pRenderer) {
        delete g_pRenderer;
        g_pRenderer = nullptr;
    }

    return (int) msg.wParam;
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
    DWORD dwExStyle = 0;
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
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
