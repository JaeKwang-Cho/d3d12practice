// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include <windowsx.h>
#include <filesystem>
#include "D3D12Renderer.h"

// D3D 라이브러리 링크
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment( lib, "d3d11.lib" ) 

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
WCHAR g_tempPath[MAX_PATH];

D3D12Renderer* g_pRenderer = nullptr;

ULONGLONG g_PrvFrameCheckTick = 0;
ULONGLONG g_PrvUpdateTick = 0;
DWORD	g_FrameCount = 0;

// ===============================

bool g_bShiftKeyDown = false;
float g_CamOffsetX = 0.0f;
float g_CamOffsetY = 0.0f;
float g_CamOffsetZ = 0.0f;

// Delta Time
ULONGLONG g_PrvGameLoopTick = 0;
float g_fDeltaTime = 0.f;

constexpr float CAM_MOVE_SPEED      = 1.0f;   // units/sec
constexpr float CAM_ROT_SENSITIVITY = 0.003f;  // rad/pixel

// 마우스 델타 누적 (OnMouseMove → RunGame에서 소비)
int g_iAccumMouseDeltaX = 0;
int g_iAccumMouseDeltaY = 0;

bool g_bMouseLButtonDown = false;
bool g_bMouseMButtonDown = false;
bool g_bMouseRButtonDown = false;
bool g_bCamRotMode = false;
int g_iMouseX_RButtonPressed = 0;
int g_iMouseY_RButtonPressed = 0;
int g_iPrvMouseX = 0;
int g_iPrvMouseY = 0;
int g_iCurMouseX = 0;
int g_iCurMouseY = 0;

void OnKeyDown(UINT _nChar, UINT _uiScanCode);
void OnKeyUp(UINT _nChar, UINT _uiScanCode);
void OnMouseLButtonDown(int _x, int _y, UINT _nFlags);
void OnMouseLButtonUp(int _x, int _y, UINT _nFlags);
void OnMouseRButtonDown(int _x, int _y, UINT _nFlags);
void OnMouseRButtonUp(int _x, int _y, UINT _nFlags);
void OnMouseMButtonDown(int _x, int _y, UINT _nFlags);
void OnMouseMButtonUp(int _x, int _y, UINT _nFlags);
void OnMouseMove(int _x, int _y, UINT _nFlags);
void OnMouseWheel(int _x, int _y, int _iWheel);
void OnMouseHWheel(int _x, int _y, int _iWheel);

// ===============================

// 윈도우 프로시져
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 게임 루프
void RunGame();
void Update();

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

    // COM 초기화
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        __debugbreak();
    }

    MSG msg = {};

    WCHAR wchAppPath[_MAX_PATH];
    GetCurrentDirectory(_MAX_PATH, wchAppPath);

    WCHAR wchExePath[_MAX_PATH];
    GetModuleFileNameW(nullptr, wchExePath, _MAX_PATH);

    std::filesystem::path shaderPath =
        std::filesystem::path(wchExePath).parent_path() / L"..\\Shaders";
    shaderPath = std::filesystem::canonical(shaderPath);

    g_pRenderer = new D3D12Renderer;
    g_pRenderer->Initialize(g_hWnd, true, true, true, shaderPath.wstring().c_str());

    // 기본으로 PeekMessage를 사용
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            RunGame();
        }
    }

    if (g_pRenderer)
    {
        delete g_pRenderer;
        g_pRenderer = nullptr;
    }

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
#ifdef _DEBUG
    // 여기서 걸리면 위에서 지정해놓은 디버기 플래그에 해당하는 문제가 터진 것이다.
    // 디버그 힙에서 콜스택을 추적해준다.
    _ASSERT(_CrtCheckMemory());
#endif

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // 마우스 좌표는 switch 전에 한 번만 추출
    const int iMouseX = GET_X_LPARAM(lParam);
    const int iMouseY = GET_Y_LPARAM(lParam);

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
        if (g_pRenderer)
        {
            RECT	rect;
            GetClientRect(hWnd, &rect);
            DWORD	dwWndWidth = rect.right - rect.left;
            DWORD	dwWndHeight = rect.bottom - rect.top;
            g_pRenderer->UpdateWindowSize(dwWndWidth, dwWndHeight);
        }
    }
    break;
    case WM_MOUSEMOVE:
        if (g_pRenderer)
            OnMouseMove(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_LBUTTONDOWN:
        if (g_pRenderer)
            OnMouseLButtonDown(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_LBUTTONUP:
        if (g_pRenderer)
            OnMouseLButtonUp(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_RBUTTONDOWN:
        if (g_pRenderer)
            OnMouseRButtonDown(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_RBUTTONUP:
        if (g_pRenderer)
            OnMouseRButtonUp(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_MBUTTONDOWN:
        if (g_pRenderer)
            OnMouseMButtonDown(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_MBUTTONUP:
        if (g_pRenderer)
            OnMouseMButtonUp(iMouseX, iMouseY, (UINT)wParam);
        break;
    case WM_KEYDOWN:
    {
        if (g_pRenderer)
        {
            UINT	uiScanCode = (0x00ff0000 & lParam) >> 16;
            UINT	vkCode = MapVirtualKey(uiScanCode, MAPVK_VSC_TO_VK);
            if (!(lParam & 0x40000000))
            {
                OnKeyDown(vkCode, uiScanCode);
            }
        }
    }
    break;
    case WM_KEYUP:
    {
        if (g_pRenderer)
        {
            UINT	uiScanCode = (0x00ff0000 & lParam) >> 16;
            UINT	vkCode = MapVirtualKey(uiScanCode, MAPVK_VSC_TO_VK);
            OnKeyUp(vkCode, uiScanCode);
        }
    }
    break;
    case WM_DROPFILES:
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void RunGame()
{
    g_FrameCount++;

    // Delta time 계산
    ULONGLONG CurTick = GetTickCount64();
    if (g_PrvGameLoopTick > 0)
    {
        g_fDeltaTime = static_cast<float>(CurTick - g_PrvGameLoopTick) * 0.001f;
        g_fDeltaTime = fmin(g_fDeltaTime, 0.05f); // 최대 50ms 상한
    }
    g_PrvGameLoopTick = CurTick;

    // 카메라 이동 — delta time 적용
    if (g_CamOffsetX != 0.0f || g_CamOffsetY != 0.0f || g_CamOffsetZ != 0.0f)
    {
        g_pRenderer->MoveCamera(
            g_CamOffsetX * CAM_MOVE_SPEED * g_fDeltaTime,
            g_CamOffsetY * CAM_MOVE_SPEED * g_fDeltaTime,
            g_CamOffsetZ * CAM_MOVE_SPEED * g_fDeltaTime
        );
    }

    // 카메라 회전 — 누적 마우스 델타 소비 (delta time 불필요: 픽셀 이동량이 이미 독립적)
    if (g_iAccumMouseDeltaX != 0 || g_iAccumMouseDeltaY != 0)
    {
        float fYaw   = static_cast<float>(g_iAccumMouseDeltaX) * CAM_ROT_SENSITIVITY;
        float fPitch = static_cast<float>(g_iAccumMouseDeltaY) * CAM_ROT_SENSITIVITY;
        g_pRenderer->ApplyCameraRot(fYaw, fPitch, 0.f);
        g_iAccumMouseDeltaX = 0;
        g_iAccumMouseDeltaY = 0;
    }

    g_pRenderer->BeginRender();

    if (CurTick - g_PrvUpdateTick > 16)
    {
        Update();
        g_PrvUpdateTick = CurTick;
    }

    g_pRenderer->EndRender();
    g_pRenderer->Present();

    if (CurTick - g_PrvFrameCheckTick > 1000)
    {
        g_PrvFrameCheckTick = CurTick;

        WCHAR wchTxt[64];
        swprintf_s(wchTxt, L"FPS:%u", g_FrameCount);
        SetWindowText(g_hWnd, wchTxt);

        g_FrameCount = 0;
    }
}

void Update()
{
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

void OnKeyDown(UINT _nChar, UINT _uiScanCode)
{
    switch (_nChar)
    {
    case VK_SHIFT:
        g_bShiftKeyDown = TRUE;
        break;
    case 'W':
        g_CamOffsetZ = 1.0f;
        break;
    case 'S':
        g_CamOffsetZ = -1.0f;
        break;
    case 'A':
        g_CamOffsetX = -1.0f;
        break;
    case 'D':
        g_CamOffsetX = 1.0f;
        break;
    case 'Q':
        g_CamOffsetY = 1.0f;
        break;
    case 'E':
        g_CamOffsetY = -1.0f;
		break;
    }
}

void OnKeyUp(UINT _nChar, UINT _uiScanCode)
{
    switch (_nChar)
    {
    case VK_SHIFT:
        g_bShiftKeyDown = FALSE;
        break;
    case 'W':
        g_CamOffsetZ = 0.0f;
        break;
    case 'S':
        g_CamOffsetZ = 0.0f;
        break;
    case 'A':
        g_CamOffsetX = 0.0f;
        break;
    case 'D':
        g_CamOffsetX = 0.0f;
        break;
	case 'Q':
        g_CamOffsetY = 0.0f;
		break;
    case 'E':
        g_CamOffsetY = 0.0f;
		break;
    }
}

void OnMouseLButtonDown(int _x, int _y, UINT _nFlags)
{
	g_bMouseLButtonDown = true;
}

void OnMouseLButtonUp(int _x, int _y, UINT _nFlags)
{
	g_bMouseLButtonDown = false;
}

void OnMouseRButtonDown(int _x, int _y, UINT _nFlags)
{
	g_bCamRotMode = true;
	g_iMouseX_RButtonPressed = _x;
	g_iMouseY_RButtonPressed = _y;

	g_bMouseRButtonDown = true;
}

void OnMouseRButtonUp(int _x, int _y, UINT _nFlags)
{
    g_bCamRotMode = false;
	g_bMouseRButtonDown = false;
}

void OnMouseMButtonDown(int _x, int _y, UINT _nFlags)
{
	g_bMouseMButtonDown = true;
}

void OnMouseMButtonUp(int _x, int _y, UINT _nFlags)
{
	g_bMouseMButtonDown = false;
}

void OnMouseMove(int _x, int _y, UINT _nFlags)
{
    g_iPrvMouseX = g_iCurMouseX;
    g_iPrvMouseY = g_iCurMouseY;

    int dx = _x - g_iPrvMouseX;
    int dy = _y - g_iPrvMouseY;

    // ApplyCameraRot를 직접 호출하지 않고, RunGame에서 일괄 처리
    if (g_bCamRotMode)
    {
        g_iAccumMouseDeltaX += dx;
        g_iAccumMouseDeltaY += dy;
    }
    g_iCurMouseX = _x;
    g_iCurMouseY = _y;
}

void OnMouseWheel(int _x, int _y, int _iWheel)
{
}

void OnMouseHWheel(int _x, int _y, int _iWheel)
{
}
