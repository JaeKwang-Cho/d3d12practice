// 01_VerySimpleArchitecture.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "D3D12Renderer.h"
#include "VertexUtil.h"
// 기본 에셋
#include "CommonAssets.h"
#include "Grid_RenderMesh.h"
#include "TextureRenderMesh.h"

#include <windowsx.h>
#include <shellapi.h> // 파일 드래그앤드롭

// COM
#include <combaseapi.h>


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
extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\x64\\"; } // c++20 이므로 이렇게 해줘야 한다.
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
D3D12_HEAP_PROPERTIES HEAP_PROPS_READBACK = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

// 렌더링 전역 변수
D3D12Renderer* g_pRenderer = nullptr;

void* g_pGrid = nullptr;
void* g_pCube = nullptr;

XMMATRIX g_matWorldGrid = {};
XMMATRIX g_matWorldCube = {};

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
void* CreateCube(float _width, float _height, float _depth);

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

    // COM 초기화
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        __debugbreak();
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
    g_pGrid = CreateTileGrid();
    g_pCube = CreateCube(10.f, 5.f, 2.f);

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
    g_pRenderer->FlushMultiRendering();

    DeleteCommonAssets(g_pRenderer);


    if (g_pGrid) {
        Grid_RenderMesh* pGrid = reinterpret_cast<Grid_RenderMesh*>(g_pGrid);
        delete pGrid;
        //g_pRenderer->DeleteRenderMesh(g_pGrid, E_RENDER_MESH_TYPE::COLOR);
        g_pGrid = nullptr;
    }
    if (g_pCube) {
        TextureRenderMesh* pCube = reinterpret_cast<TextureRenderMesh*>(g_pCube);
        delete pCube;
        //g_pRenderer->DeleteRenderMesh(g_pCube, E_RENDER_MESH_TYPE::TEXTURE);
        pCube = nullptr;
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
        g_pRenderer->TryPixelStreaming();
        g_PrevUpdateTime = CurrTickTime;
    }
   
    // ===== draw object =====
    g_pRenderer->DrawRenderMesh(g_pGrid, &g_matWorldGrid, E_RENDER_MESH_TYPE::COLOR);

    g_pRenderer->DrawRenderMesh(g_pCube, &g_matWorldCube, E_RENDER_MESH_TYPE::TEXTURE);

    g_pRenderer->DrawOutlineMesh(g_pCube, &g_matWorldCube);
    // =======================

    // copy render target
    g_pRenderer->CopyRenderTarget();

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
    g_matWorldCube = XMMatrixIdentity();
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

    Grid_RenderMesh* pNewGrid = new Grid_RenderMesh;
    pNewGrid->Initialize(g_pRenderer, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    pNewGrid->CreateRenderAssets(meshData, 1);

    return pNewGrid;
}

void* CreateCube(float _width, float _height, float _depth)
{
    std::vector<TextureMeshData> meshData;
    meshData.push_back(TextureMeshData());

    TextureMeshData& refMeshData = meshData[0];

    // adjacency test
    std::vector<XMFLOAT3> posL;

    float w2 = 0.5f * _width;
    float h2 = 0.5f * _height;
    float d2 = 0.5f * _depth;

    refMeshData.Vertices.resize(24);
    posL.resize(24);
    // 앞면 (pos, norm, tangent, UVc 순)
    refMeshData.Vertices[0] = TextureVertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    refMeshData.Vertices[1] = TextureVertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    refMeshData.Vertices[2] = TextureVertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    refMeshData.Vertices[3] = TextureVertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    posL[0] = XMFLOAT3(-w2, -h2, -d2);
    posL[1] = XMFLOAT3(-w2, +h2, -d2);
    posL[2] = XMFLOAT3(+w2, +h2, -d2);
    posL[3] = XMFLOAT3(+w2, -h2, -d2);

    // 뒷면 (pos, norm, tangent, UVc 순)
    refMeshData.Vertices[4] = TextureVertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    refMeshData.Vertices[5] = TextureVertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    refMeshData.Vertices[6] = TextureVertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    refMeshData.Vertices[7] = TextureVertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    posL[4] = XMFLOAT3(-w2, -h2, +d2);
    posL[5] = XMFLOAT3(+w2, -h2, +d2);
    posL[6] = XMFLOAT3(+w2, +h2, +d2);
    posL[7] = XMFLOAT3(-w2, +h2, +d2);

    // 윗면 (pos, norm, tangent, UVc 순)
    refMeshData.Vertices[8] = TextureVertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    refMeshData.Vertices[9] = TextureVertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    refMeshData.Vertices[10] =TextureVertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    refMeshData.Vertices[11] =TextureVertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    posL[8]  = XMFLOAT3(-w2, +h2, -d2);
    posL[9]  = XMFLOAT3(-w2, +h2, +d2);
    posL[10] = XMFLOAT3(+w2, +h2, +d2);
    posL[11] = XMFLOAT3(+w2, +h2, -d2);

    // 밑면 (pos, norm, tangent, UVc 순)
    refMeshData.Vertices[12] = TextureVertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    refMeshData.Vertices[13] = TextureVertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    refMeshData.Vertices[14] = TextureVertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    refMeshData.Vertices[15] = TextureVertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    posL[12] = XMFLOAT3(-w2, -h2, -d2);
    posL[13] = XMFLOAT3(+w2, -h2, -d2);
    posL[14] = XMFLOAT3(+w2, -h2, +d2);
    posL[15] = XMFLOAT3(-w2, -h2, +d2);

    // 왼면 (pos, norm, tangent, UVc 순)
    refMeshData.Vertices[16] = TextureVertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    refMeshData.Vertices[17] = TextureVertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    refMeshData.Vertices[18] = TextureVertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    refMeshData.Vertices[19] = TextureVertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

    posL[16] = XMFLOAT3(-w2, -h2, +d2);
    posL[17] = XMFLOAT3(-w2, +h2, +d2);
    posL[18] = XMFLOAT3(-w2, +h2, -d2);
    posL[19] = XMFLOAT3(-w2, -h2, -d2);

    // 오른면 (pos, norm, tangent, UVc 순)
    refMeshData.Vertices[20] = TextureVertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    refMeshData.Vertices[21] = TextureVertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    refMeshData.Vertices[22] = TextureVertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    refMeshData.Vertices[23] = TextureVertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    posL[20] = XMFLOAT3(+w2, -h2, -d2);
    posL[21] = XMFLOAT3(+w2, +h2, -d2);
    posL[22] = XMFLOAT3(+w2, +h2, +d2);
    posL[23] = XMFLOAT3(+w2, -h2, +d2);

    // Index 버퍼를 만든다.

    refMeshData.Indices32.resize(36);
    // 전면 인덱스
    refMeshData.Indices32[0] = 0; refMeshData.Indices32[1] = 1; refMeshData.Indices32[2] = 2;
    refMeshData.Indices32[3] = 0; refMeshData.Indices32[4] = 2; refMeshData.Indices32[5] = 3;

    // 후면 인덱스
    refMeshData.Indices32[6] = 4; refMeshData.Indices32[7] = 5; refMeshData.Indices32[8] = 6;
    refMeshData.Indices32[9] = 4; refMeshData.Indices32[10] = 6; refMeshData.Indices32[11] = 7;

    // 윗면 인덱스
    refMeshData.Indices32[12] = 8; refMeshData.Indices32[13] = 9; refMeshData.Indices32[14] = 10;
    refMeshData.Indices32[15] = 8; refMeshData.Indices32[16] = 10; refMeshData.Indices32[17] = 11;

    // 밑면 인덱스
    refMeshData.Indices32[18] = 12; refMeshData.Indices32[19] = 13; refMeshData.Indices32[20] = 14;
    refMeshData.Indices32[21] = 12; refMeshData.Indices32[22] = 14; refMeshData.Indices32[23] = 15;

    // 왼면 인덱스
    refMeshData.Indices32[24] = 16; refMeshData.Indices32[25] = 17; refMeshData.Indices32[26] = 18;
    refMeshData.Indices32[27] = 16; refMeshData.Indices32[28] = 18; refMeshData.Indices32[29] = 19;

    // 오른면 인덱스
    refMeshData.Indices32[30] = 20; refMeshData.Indices32[31] = 21; refMeshData.Indices32[32] = 22;
    refMeshData.Indices32[33] = 20; refMeshData.Indices32[34] = 22; refMeshData.Indices32[35] = 23;

    TextureRenderMesh* pNewCube = new TextureRenderMesh;
    pNewCube->Initialize(g_pRenderer);

    // adjacency test
    std::vector<uint32_t> adjIndices;
    GenerateAdjacencyIndices(posL, refMeshData.Indices32, adjIndices);

    pNewCube->CreateRenderAssets(meshData, 1, adjIndices);
    CONSTANT_BUFFER_MATERIAL purpleMat = CONSTANT_BUFFER_MATERIAL(XMFLOAT4(0.5f, 0.2f, 0.7f, 1.f));
    pNewCube->SetMaterial(purpleMat, 0);
    
    return pNewCube;
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
