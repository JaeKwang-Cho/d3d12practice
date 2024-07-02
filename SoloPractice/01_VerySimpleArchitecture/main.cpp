// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "D3D12Renderer.h"
#include "VertexUtil.h"

// D3D 라이브러리 링크
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// DirectX Tex 설정
// https://github.com/microsoft/DirectXTex
#if defined(_M_AMD64)
#ifdef _DEBUG
#pragma comment(lib, "../../DirectXTex/DirectXTex/Bin/Desktop_2022/x64/debug/DirectXTex.lib")
#else
#pragma comment(lib, "../../DirectXTex/DirectXTex/Bin/Desktop_2022/x64/release/DirectXTex.lib")
#endif
#endif

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
void* g_pMeshObj0 = nullptr;
void* g_pMeshObj1 = nullptr;

void* g_pSpriteObjCommon = nullptr;
void* g_pSpriteObj0 = nullptr;
void* g_pSpriteObj1 = nullptr;
void* g_pSpriteObj2 = nullptr;
void* g_pSpriteObj3 = nullptr;
void* g_pTexHandle0 = nullptr;
void* g_pTexHandle1 = nullptr;
 
float g_fRot0 = 0.0f;
float g_fRot1 = 0.0f;
float g_fRot2 = 0.0f;

XMMATRIX g_matWorld0 = {};
XMMATRIX g_matWorld1 = {};
XMMATRIX g_matWorld2 = {};

// test
float g_fOffsetX = 0.f;
float g_fOffsetY = 0.f;
float g_fSpeed = 0.01f;

// Tick Time
ULONGLONG g_PrvFrameTick = 0;
ULONGLONG g_PrvUpdateTick = 0;
DWORD	g_FrameCount = 0;
 
// 렌더링 함수
void RunGame();
void Update();

// 임시 함수
void* CreateBoxMeshObject();
void* CreateQuadMesh();

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

    // main에서 Box Mesh를 미리 만들기
    g_pMeshObj0 = CreateBoxMeshObject();
    // main에서 Quad Mesh를 미리 만들기
    g_pMeshObj1 = CreateQuadMesh();

    // sprite를 만든다. 이 구조에서 단점은 sprite를 만들 texture file의 정보를 알고 있어야 한다는 것
    g_pTexHandle0 = g_pRenderer->CreateTextureFromFile(L"../../Assets/salt.dds");
    g_pTexHandle1 = g_pRenderer->CreateTextureFromFile(L"../../Assets/salt.png");
    g_pSpriteObjCommon = g_pRenderer->CreateSpriteObject();

    g_pSpriteObj0 = g_pRenderer->CreateSpriteObject(L"../../Assets/salt_01.dds", 0, 0, 640, 640);
    g_pSpriteObj1 = g_pRenderer->CreateSpriteObject(L"../../Assets/salt_01.dds", 640, 0, 1280, 640);
    g_pSpriteObj2 = g_pRenderer->CreateSpriteObject(L"../../Assets/salt_01.dds", 0, 640, 640, 1280);
    g_pSpriteObj3 = g_pRenderer->CreateSpriteObject(L"../../Assets/salt_01.dds", 640, 640, 1280, 1280);


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

    if (g_pMeshObj0) {
        g_pRenderer->DeleteBasicMeshObject(g_pMeshObj0);
        g_pMeshObj0 = nullptr;
    }
    if (g_pMeshObj1) {
        g_pRenderer->DeleteBasicMeshObject(g_pMeshObj1);
        g_pMeshObj1 = nullptr;
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
    if (g_pSpriteObjCommon)
    {
        g_pRenderer->DeleteSpriteObject(g_pSpriteObjCommon);
        g_pSpriteObjCommon = nullptr;
    }
    if (g_pSpriteObj0)
    {
        g_pRenderer->DeleteSpriteObject(g_pSpriteObj0);
        g_pSpriteObj0 = nullptr;
    }
    if (g_pSpriteObj1)
    {
        g_pRenderer->DeleteSpriteObject(g_pSpriteObj1);
        g_pSpriteObj1 = nullptr;
    }
    if (g_pSpriteObj2)
    {
        g_pRenderer->DeleteSpriteObject(g_pSpriteObj2);
        g_pSpriteObj2 = nullptr;
    }
    if (g_pSpriteObj3)
    {
        g_pRenderer->DeleteSpriteObject(g_pSpriteObj3);
        g_pSpriteObj3 = nullptr;
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
    ULONGLONG CurTick = GetTickCount64();
    g_pRenderer->BeginRender();

    // game business logic
    if (CurTick - g_PrvUpdateTick > 16)
    {
        // Update Scene with 60FPS
        Update();
        g_PrvUpdateTick = CurTick;
    }

    // rendering objects
    // cube에 대해서 2번 렌더링을 다르게 한다.
    g_pRenderer->RenderMeshObject(g_pMeshObj0, &g_matWorld0);
    g_pRenderer->RenderMeshObject(g_pMeshObj0, &g_matWorld1);
    // quad를 그린다.
    g_pRenderer->RenderMeshObject(g_pMeshObj1, &g_matWorld2);

    // sprite를 그린다.
    int padding = 5;
    
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = 540;
    rect.bottom = 540;
    
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 0, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle1);
    
    rect.left = 540;
    rect.top = 0;
    rect.right = 1080;
    rect.bottom = 540;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 270 + padding, 0, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle0);

    rect.left = 0;
    rect.top = 540;
    rect.right = 540;
    rect.bottom = 1080;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 270 + padding, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle0);

    rect.left = 540;
    rect.top = 540;
    rect.right = 1080;
    rect.bottom = 1080;
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 270 + padding, 270 + padding, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle0);
    
    g_pRenderer->RenderSprite(g_pSpriteObj0, 540 + padding + padding, 0, 0.25f, 0.25f, 0.f);
    g_pRenderer->RenderSprite(g_pSpriteObj1, 540 + padding + padding + 160 + padding, 0, 0.25f, 0.25f, 0.f);
    g_pRenderer->RenderSprite(g_pSpriteObj2, 540 + padding + padding, 160+ padding, 0.25f, 0.25f, 0.f);
    g_pRenderer->RenderSprite(g_pSpriteObj3, 540 + padding + padding + 160 + padding, 160 + padding, 0.25f, 0.25f, 0.f);
    

    // end
    g_pRenderer->EndRender();

    // Present
    g_pRenderer->Present();

    // FPS
    if (CurTick - g_PrvFrameTick > 1000)
    {
        g_PrvFrameTick = CurTick;

        WCHAR wchTxt[64];
        swprintf_s(wchTxt, L"FPS:%u", g_FrameCount);

        g_FrameCount = 0;
    }

}

void Update()
{
    g_matWorld0 = XMMatrixIdentity();
    XMMATRIX matRot0 = XMMatrixRotationX(g_fRot0);
    XMMATRIX matTrans0 = XMMatrixTranslation(-0.66f, 0.0f, 0.66f);

    g_matWorld0 = XMMatrixMultiply(matRot0, matTrans0);

    g_matWorld1 = XMMatrixIdentity();
    XMMATRIX matRot1 = XMMatrixRotationY(g_fRot1);
    XMMATRIX matTrans1 = XMMatrixTranslation(0.f, 0.0f, 0.66f);

    g_matWorld1 = XMMatrixMultiply(matRot1, matTrans1);

    g_matWorld2 = XMMatrixIdentity();

    XMMATRIX matRot2 = XMMatrixRotationZ(g_fRot2);
    XMMATRIX matTrans2 = XMMatrixTranslation(0.66f, 0.0f, 0.66f);

    g_matWorld2 = XMMatrixMultiply(matRot2, matTrans2);

    bool bChangeTex = false;
    g_fRot0 += 0.05f;
    if (g_fRot0 > 2.0f * XM_PI)
    {
        g_fRot0 = 0.0f;
        bChangeTex = TRUE;
    }

    g_fRot1 += 0.1f;
    if (g_fRot1 > 2.0f * XM_PI)
    {
        g_fRot1 = 0.0f;
    }

    g_fRot2 += 0.1f;
    if (g_fRot2 > 2.0f * XM_PI)
    {
        g_fRot2 = 0.0f;
    }
}

void* CreateBoxMeshObject()
{
    void* pMeshObj = nullptr;

    // 도우미 함수에서 적당히 Box를 만든다.
    uint16_t	pIndexList[36] = {};
    BasicVertex* pVertexList = nullptr;
    DWORD dwVertexCount = CreateBoxMesh(&pVertexList, pIndexList, (DWORD)_countof(pIndexList), 0.25f);

    // Vertex 정보가 없는 빈 인스턴스를 만든다.
    pMeshObj = g_pRenderer->CreateBasicMeshObject();

    const WCHAR* wchTexFileNameList[6] =
    {
        L"../../Assets/salt_01.dds",
        L"../../Assets/salt_02.dds",
        L"../../Assets/salt_03.dds",
        L"../../Assets/salt_04.dds",
        L"../../Assets/salt_05.dds",
        L"../../Assets/salt_06.dds"
    };

    // 박스 면마다 텍스쳐를 다르게 한다.
    g_pRenderer->BeginCreateMesh(pMeshObj, pVertexList, dwVertexCount, 6);	// 박스의 6면-1면당 삼각형 2개-인덱스 6개
    for (DWORD i = 0; i < 6; i++)
    {
        // 삼각형 2개마다 하나의 텍스쳐를 이용하게 된다.
        g_pRenderer->InsertTriGroup(pMeshObj, pIndexList + i * 6, 2, wchTexFileNameList[i]);
    }
    g_pRenderer->EndCreateMesh(pMeshObj);

    // Vertex Data가 넘어갔으니, 해제 해준다.
    if (pVertexList)
    {
        DeleteBoxMesh(pVertexList);
        pVertexList = nullptr;
    }
    return pMeshObj;
}

void* CreateQuadMesh()
{
    void* pMeshObj = nullptr;
    pMeshObj = g_pRenderer->CreateBasicMeshObject();

    // 사각형 정보이다.
    BasicVertex pVertexList[] =
    {
        { { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
        { { 0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        { { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    };

    uint16_t pIndexList[] =
    {
        0, 1, 2,
        0, 2, 3
    };

    // 사각형이고 점 4개 인덱스 6개, 삼각형 2개가 하나의 텍스쳐를 받아서 그린다.
    g_pRenderer->BeginCreateMesh(pMeshObj, pVertexList, (DWORD)_countof(pVertexList), 1);
    g_pRenderer->InsertTriGroup(pMeshObj, pIndexList, 2, L"../../Assets/salt.dds");
    g_pRenderer->EndCreateMesh(pMeshObj);
    return pMeshObj;
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
