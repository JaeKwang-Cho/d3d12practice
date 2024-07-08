﻿// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "D3D12Renderer.h"
#include "VertexUtil.h"
// 기본 에셋
#include "CommonAssets.h"
#include "Grid_RenderMesh.h"
#include <windowsx.h>
#include <shellapi.h> // 파일 드래그앤드롭

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
WCHAR g_tempPath[MAX_PATH];

// D3D12 전역변수
D3D12_HEAP_PROPERTIES HEAP_PROPS_DEFAULT = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
D3D12_HEAP_PROPERTIES HEAP_PROPS_UPLOAD = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

// 렌더링 전역 변수
D3D12Renderer* g_pRenderer = nullptr;
void* g_pMeshObj0 = nullptr;
void* g_pMeshObj1 = nullptr;

void* g_pGrid = nullptr;

XMMATRIX g_matWorldGrid = {};

// Tick Time
ULONGLONG g_PrevFrameTime = 0;
ULONGLONG g_PrevUpdateTime = 0;
DWORD	g_FrameCount = 0;
float g_DeltaTime = 0;

GameTimer g_GameTimer;
 
// 렌더링 함수
void RunGame();
void Update();

// 임시 함수
void* CreateTileGrid();

UINT g_GridCellOffset = 0;
void UpdateGridPos();

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
    // Timer 초기화
    g_GameTimer.Reset();
    g_GameTimer.Start();

    // D3D 초기화
    g_pRenderer = new D3D12Renderer;
    g_pRenderer->Initialize(g_hWnd, true, true);

    // Common Assets 초기화
    CreateCommonAssets(g_pRenderer);

    // main 에서 grid mesh를 미리 만들기
    //g_pGrid = CreateGrid(100, 100, 50, 50);
    g_pGrid = CreateTileGrid();


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
    g_GameTimer.Stop();
    DeleteCommonAssets(g_pRenderer);

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
    // Tick
    g_FrameCount++;
    g_GameTimer.Tick();
    // begin
    g_pRenderer->Update(g_GameTimer);
    g_pRenderer->BeginRender();

    // game business logic
    ULONGLONG CurrTickTime = GetTickCount64();
    if (CurrTickTime - g_PrevUpdateTime > 16)
    {
        // Update Scene with 60FPS
        Update();
        g_PrevUpdateTime = CurrTickTime;
    }
   
    // rendering objects
    // cube에 대해서 2번 렌더링을 다르게 한다.
    //g_pRenderer->RenderMeshObject(g_pMeshObj0, &g_matWorld0);
    //g_pRenderer->RenderMeshObject(g_pMeshObj0, &g_matWorld1);
    // quad를 그린다.
    //g_pRenderer->RenderMeshObject(g_pMeshObj1, &g_matWorld2);

    // sprite를 그린다.
    int padding = 5;
    
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = 540;
    rect.bottom = 540;
    
    //g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 0, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle1);
    
    rect.left = 540;
    rect.top = 0;
    rect.right = 1080;
    rect.bottom = 540;
    //g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 270 + padding, 0, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle0);

    rect.left = 0;
    rect.top = 540;
    rect.right = 540;
    rect.bottom = 1080;
    //g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 270 + padding, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle1);

    rect.left = 540;
    rect.top = 540;
    rect.right = 1080;
    rect.bottom = 1080;
    //g_pRenderer->RenderSpriteWithTex(g_pSpriteObjCommon, 270 + padding, 270 + padding, 0.25f, 0.25f, &rect, 0.f, g_pTexHandle0);
    
    //g_pRenderer->RenderSprite(g_pSpriteObj0, 540 + padding + padding, 0, 0.25f, 0.25f, 0.f);
    //g_pRenderer->RenderSprite(g_pSpriteObj1, 540 + padding + padding + 160 + padding, 0, 0.25f, 0.25f, 0.f);
    //g_pRenderer->RenderSprite(g_pSpriteObj2, 540 + padding + padding, 160+ padding, 0.25f, 0.25f, 0.f);
    //g_pRenderer->RenderSprite(g_pSpriteObj3, 540 + padding + padding + 160 + padding, 160 + padding, 0.25f, 0.25f, 0.f);
    
    
    //XMMATRIX identitiy = XMMatrixIdentity();
    g_pRenderer->DrawRenderMesh(g_pGrid, &g_matWorldGrid);

    // end
    g_pRenderer->EndRender();

    // Present
    g_pRenderer->Present();

    // FPS
    if (CurrTickTime - g_PrevFrameTime > 1000)
    {
        g_PrevFrameTime = CurrTickTime;

        WCHAR wchTxt[64];
        swprintf_s(wchTxt, L"FPS:%u Delta:%f", g_FrameCount, g_GameTimer.GetDeltaTime());
        SetWindowText(g_hWnd, wchTxt);
        g_FrameCount = 0;
    }
}

void Update()
{
    UpdateGridPos();
}

void* CreateTileGrid()
{
    std::vector<ColorMeshData> meshData;
    meshData.push_back(ColorMeshData());

    // 간격이 너무 좁은것 같아서 넓혀주었다.
    int vertexCount = 11;
    g_GridCellOffset = 25.f;

    // -x+, -y+ 번갈아 가면서 넣어주고
    ColorMeshData& refMeshData = meshData[0];

    refMeshData.Vertices.resize(vertexCount * 2);
    refMeshData.Indices32.resize(vertexCount * 2);
    for (int i = 0; i < vertexCount; i++)
    {
        int curIndex = i * 2;
        refMeshData.Vertices[curIndex].position = XMFLOAT3(float(i - vertexCount / 2) * g_GridCellOffset, 0.f , 0.f);
        refMeshData.Vertices[curIndex].color = XMFLOAT4(DirectX::Colors::DarkRed);
        refMeshData.Vertices[curIndex].texCoord = XMFLOAT2(0.f, 0.f); // 텍스쳐는 입히지 않는다.

        refMeshData.Vertices[curIndex + 1].position = XMFLOAT3(0.f, 0.f, float(i - vertexCount / 2) * g_GridCellOffset);
        refMeshData.Vertices[curIndex + 1].color = XMFLOAT4(DirectX::Colors::DarkGreen);
        refMeshData.Vertices[curIndex + 1].texCoord = XMFLOAT2(0.f, 0.f); // 텍스쳐는 입히지 않는다.

        // 인덱스도 적당히 짝지어주는 거로 넘긴다.
        refMeshData.Indices32[curIndex] = curIndex;
        refMeshData.Indices32[curIndex + 1] = curIndex + 1;
    }

    Grid_RenderMesh* newGrid = new Grid_RenderMesh;
    newGrid->Initialize(g_pRenderer, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    newGrid->CreateRenderAssets(meshData, 1);

    return newGrid;
}


//CHAR wchTxt[64];
//sprintf_s(wchTxt, "Camera Pos (%f, %f)\n", curCameraPos.x, curCameraPos.z);
//OutputDebugStringA(wchTxt);

void UpdateGridPos()
{
    XMFLOAT3 curCameraPos = g_pRenderer->GetCameraWorldPos();

    UINT xOffset = curCameraPos.x / g_GridCellOffset;
    UINT zOffset = curCameraPos.z / g_GridCellOffset;
    
    g_matWorldGrid = XMMatrixTranslation(xOffset * g_GridCellOffset, 0.f, zOffset * g_GridCellOffset);
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
    case WM_LBUTTONDOWN:
        break;
    case WM_MBUTTONDOWN:
        break;
    case WM_RBUTTONDOWN:
    {
        if (g_pRenderer) {
            g_pRenderer->OnRButtonDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
    }
    return 0;
    case WM_LBUTTONUP:
        break;
    case WM_MBUTTONUP:
        break;
    case WM_RBUTTONUP:
    {
        if (g_pRenderer) {
            g_pRenderer->OnRButtonUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
    } 
    return 0;
    case WM_MOUSEMOVE:
    {
        if (g_pRenderer) {
            g_pRenderer->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
    }
    return 0;
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        UINT fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
        if (fileCount >= 2) {
            MessageBox(g_hWnd, L"일단은 파일 하나만", L"경고", MB_OK);
            return 0;
        }
        UINT filePathLength = DragQueryFileW(hDrop, 0, g_tempPath, MAX_PATH);
        OutputDebugString(L"DropFile\n");
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
