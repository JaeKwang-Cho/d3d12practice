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
#include <shellapi.h>// 파일 드래그앤드롭

// COM
#include <combaseapi.h>

// LZ4
#include <lz4.h>


// D3D 라이브러리 링크
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment( lib, "d3d11.lib" ) // DWrite Font을 사용하기 위해서 D3D11도 링크한다. D3D12과 D3D11은 리소스 공유가 가능하다.

// DirectX Tex 설정
// https://github.com/microsoft/DirectXTex
#if defined(_M_AMD64)
#ifdef _DEBUG
//#pragma comment(lib, "../../DirectXTex/DirectXTex/Bin/Desktop_2022/x64/debug/DirectXTex.lib")
#else
//#pragma comment(lib, "../../DirectXTex/DirectXTex/Bin/Desktop_2022/x64/release/DirectXTex.lib")
#endif
#endif

// D3D12 Agility 설정
// 방법 : https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/#OS
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }

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

// Common Assets
void* g_pGrid = nullptr;
void* g_pCube = nullptr;
void* g_pTexCube = nullptr;

// Sprites
void* g_pSpriteObjs[5] = {nullptr,};
void* g_pTexHandleForSprites = nullptr;

// Dynamic Texture
void* g_pDynamicTexHandle = nullptr;
BYTE* g_pImage = nullptr;
UINT g_ImageWidth = 0;
UINT g_ImageHeight = 0;

// Font
BYTE* g_pTextImage = nullptr;
UINT g_TextImageWidth = 0;
UINT g_TextImageHeight = 0;
FONT_HANDLE* g_pFontObj = nullptr;
void* g_pTextTextureHandle = nullptr;
WCHAR g_wchText[64] = {};


// 큐브 각 면 텍스처 (salt_01 ~ salt_06)
TEXTURE_HANDLE* g_pCubeFaceTextures[6] = { nullptr, };

XMMATRIX g_matWorldGrid = {};
XMMATRIX g_matWorldCube = {};

// Tick Time
ULONGLONG g_PrevFrameTime = 0;
ULONGLONG g_PrevUpdateTime = 0;
ULONGLONG g_PrevStreamingTime = 0;
DWORD	g_FrameCount = 0;
DWORD g_FPS = 0;
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

    // Sprite
    g_pTexHandleForSprites = g_pRenderer->CreateTextureFromFile(L"../../Assets/tex_00.dds");
    g_pSpriteObjs[0] = g_pRenderer->CreateSpriteObject();

    g_pSpriteObjs[1] = g_pRenderer->CreateSpriteObject(L"../../Assets/sprite_1024x1024.dds", 0, 0, 512, 512);
    g_pSpriteObjs[2] = g_pRenderer->CreateSpriteObject(L"../../Assets/sprite_1024x1024.dds", 512, 0, 1024, 512);
    g_pSpriteObjs[3] = g_pRenderer->CreateSpriteObject(L"../../Assets/sprite_1024x1024.dds", 0, 512, 512, 1024);
    g_pSpriteObjs[4] = g_pRenderer->CreateSpriteObject(L"../../Assets/sprite_1024x1024.dds", 512, 512, 1024, 1024);

	// Sprite - Dynamic Texture
    g_ImageWidth = 512;
    g_ImageHeight = 256;
    g_pImage = (BYTE*)malloc(g_ImageWidth * g_ImageHeight * 4);
    DWORD* pDest = (DWORD*)g_pImage;
    for (DWORD y = 0; y < g_ImageHeight; y++)
    {
        for (DWORD x = 0; x < g_ImageWidth; x++)
        {
            pDest[x + g_ImageWidth * y] = 0xff0000ff;
        }
    }
    g_pDynamicTexHandle = g_pRenderer->CreateDynamicTexture(g_ImageWidth, g_ImageHeight);

    // Font
	std::unique_ptr<FONT_HANDLE> pFontHandle = g_pRenderer->CreateFontObject(L"Tahoma", 18.f);
	g_pFontObj = pFontHandle.get();
    g_TextImageHeight = 256;
	g_TextImageWidth = 512;
	g_pTextImage = (BYTE*)malloc(g_TextImageWidth * g_TextImageHeight * 4);
    memset(g_pTextImage, 0, g_TextImageWidth * g_TextImageHeight * 4);
	g_pTextTextureHandle = g_pRenderer->CreateDynamicTexture(g_TextImageWidth, g_TextImageHeight);


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

    for (UINT i = 0; i < 5; i++) {
        if(g_pSpriteObjs[i]) {
            g_pRenderer->DeleteSpriteObject(g_pSpriteObjs[i]);
            g_pSpriteObjs[i] = nullptr;
		}
    }

    if (g_pTexHandleForSprites) {
        g_pRenderer->DeleteTexture(g_pTexHandleForSprites);
		g_pTexHandleForSprites = nullptr;
    }

    if (g_pDynamicTexHandle)
    {
        g_pRenderer->DeleteTexture(g_pDynamicTexHandle);
        g_pDynamicTexHandle = nullptr;
    }
    if (g_pImage)
    {
        free(g_pImage);
        g_pImage = nullptr;
    }
    if(g_pTextTextureHandle) {
        g_pRenderer->DeleteTexture(g_pTextTextureHandle);
        g_pTextTextureHandle = nullptr;
	}
    if(g_pTextImage)
    {
        free(g_pTextImage);
        g_pTextImage = nullptr;
	}

    // 큐브 면 텍스처 해제
    for (UINT i = 0; i < 6; i++)
    {
        if (g_pCubeFaceTextures[i])
        {
            g_pRenderer->DeleteTexture(g_pCubeFaceTextures[i]);
            g_pCubeFaceTextures[i] = nullptr;
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

    // ===== draw object =====
    g_pRenderer->DrawRenderMesh(g_pGrid, &g_matWorldGrid, E_RENDER_MESH_TYPE::COLOR);

    g_pRenderer->DrawRenderMesh(g_pCube, &g_matWorldCube, E_RENDER_MESH_TYPE::TEXTURE);

    g_pRenderer->DrawOutlineMesh(g_pCube, &g_matWorldCube);
    // =======================

	// ===== draw sprite =====
    TEXTURE_HANDLE* pSpriteTex = reinterpret_cast<TEXTURE_HANDLE*>(g_pTexHandleForSprites);
    if (pSpriteTex)
    {
        D3D12_RESOURCE_DESC spriteTexDesc = pSpriteTex->pTexResource->GetDesc();

        LONG texWidth = static_cast<LONG>(spriteTexDesc.Width);
        LONG texHeight = static_cast<LONG>(spriteTexDesc.Height);

        float totalTime = g_GameTimer.GetTotalTime();

        auto Clamp01 = [](float v) -> float
            {
                if (v < 0.0f) return 0.1f;
                if (v > 1.0f) return 1.0f;
                return v;
            };

        // 각 sprite 하나가 샘플링할 정규화 크기
        // 0.5f면 4개를 합쳐 전체 텍스처를 모두 덮으므로 이동 없음
        // 0.25f면 4개를 합쳐 0.5 x 0.5만 덮으므로 이동 가능
        const float tileSampleWidthN = 0.125f;
        const float tileSampleHeightN = 0.125f;

        const float fullSampleWidthN = Clamp01(tileSampleWidthN * 2.0f);
        const float fullSampleHeightN = Clamp01(tileSampleHeightN * 2.0f);

        const float travelWidthN = 1.0f - fullSampleWidthN;
        const float travelHeightN = 1.0f - fullSampleHeightN;

        const float moveSpeedU = 0.20f;
        const float moveSpeedV = 0.13f;

        float sampleLeftN = 0.0f;
        float sampleTopN = 0.0f;

        if (travelWidthN > 0.0f)
        {
            sampleLeftN = fmodf(totalTime * moveSpeedU, travelWidthN);
        }

        if (travelHeightN > 0.0f)
        {
            sampleTopN = fmodf(totalTime * moveSpeedV, travelHeightN);
        }

        LONG tileSampleWidth = static_cast<LONG>(tileSampleWidthN * static_cast<float>(texWidth));
        LONG tileSampleHeight = static_cast<LONG>(tileSampleHeightN * static_cast<float>(texHeight));

        LONG sampleLeft = static_cast<LONG>(sampleLeftN * static_cast<float>(texWidth));
        LONG sampleTop = static_cast<LONG>(sampleTopN * static_cast<float>(texHeight));

        RECT rectLT = {};
        rectLT.left = sampleLeft;
        rectLT.top = sampleTop;
        rectLT.right = rectLT.left + tileSampleWidth;
        rectLT.bottom = rectLT.top + tileSampleHeight;

        RECT rectRT = {};
        rectRT.left = sampleLeft + tileSampleWidth;
        rectRT.top = sampleTop;
        rectRT.right = rectRT.left + tileSampleWidth;
        rectRT.bottom = rectRT.top + tileSampleHeight;

        RECT rectLB = {};
        rectLB.left = sampleLeft;
        rectLB.top = sampleTop + tileSampleHeight;
        rectLB.right = rectLB.left + tileSampleWidth;
        rectLB.bottom = rectLB.top + tileSampleHeight;

        RECT rectRB = {};
        rectRB.left = sampleLeft + tileSampleWidth;
        rectRB.top = sampleTop + tileSampleHeight;
        rectRB.right = rectRB.left + tileSampleWidth;
        rectRB.bottom = rectRB.top + tileSampleHeight;

        // 화면 배치 크기
        const LONG drawWidth = 128;
        const LONG drawHeight = 128;
        const LONG drawGap = 5;

        float spriteScaleX = static_cast<float>(drawWidth) / static_cast<float>(texWidth);
        float spriteScaleY = static_cast<float>(drawHeight) / static_cast<float>(texHeight);

        g_pRenderer->RenderSpriteWithTex(g_pSpriteObjs[0], 0, 0, spriteScaleX, spriteScaleY, &rectLT, 0.0f, g_pTexHandleForSprites);
        g_pRenderer->RenderSpriteWithTex(g_pSpriteObjs[0], drawWidth + drawGap, 0, spriteScaleX, spriteScaleY, &rectRT, 0.0f, g_pTexHandleForSprites);
        g_pRenderer->RenderSpriteWithTex(g_pSpriteObjs[0], 0, drawHeight + drawGap, spriteScaleX, spriteScaleY, &rectLB, 0.0f, g_pTexHandleForSprites);
        g_pRenderer->RenderSpriteWithTex(g_pSpriteObjs[0], drawWidth + drawGap, drawHeight + drawGap, spriteScaleX, spriteScaleY, &rectRB, 0.0f, g_pTexHandleForSprites);
    }

	g_pRenderer->RenderSprite(g_pSpriteObjs[1], 512 + 10, 0, 0.5f, 0.5f, 1.0f); // z가 1이므로 뒤에 그려진다.
    g_pRenderer->RenderSprite(g_pSpriteObjs[2], 512 + 10 + 10 + 256, 0, 0.5f, 0.5f, 1.0f);
    g_pRenderer->RenderSprite(g_pSpriteObjs[3], 512 + 10, 256 + 10, 0.5f, 0.5f, 0.0f); // z가 0이므로 앞에 그려진다.
    g_pRenderer->RenderSprite(g_pSpriteObjs[4], 512 + 10 + 10 + 256, 256 + 10, 0.5f, 0.5f, 0.0f); 

    // =======================

	// === draw sprite with dynamic texture ===
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjs[0], 0, 256 + 5 + 256 + 5, 0.5f, 0.5f, nullptr, 0.0f, g_pDynamicTexHandle);
    // =======================
     
	// === draw text with dynamic texture === 
     
    g_pRenderer->RenderSpriteWithTex(g_pSpriteObjs[0], 512 + 5, 256 + 5 + 256 + 5, 1.0f, 1.0f, nullptr, 0.0f, g_pTextTextureHandle);

    // ======================================
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
		g_FPS = g_FrameCount;
        swprintf_s(wchTxt, L"FPS:%u           Delta:%f", g_FrameCount, g_GameTimer.GetDeltaTime());
        SetWindowText(g_hWnd, wchTxt);
        g_FrameCount = 0;
    }
}

void Update()
{
    //g_matWorldCube = XMMatrixIdentity();
 
    { // Rotate Cube
        float totalTime = g_GameTimer.GetTotalTime();

        XMMATRIX matRotX = XMMatrixRotationX(totalTime * 0.8f);
        XMMATRIX matRotY = XMMatrixRotationY(totalTime * 1.3f);
        XMMATRIX matRotZ = XMMatrixRotationZ(totalTime * 0.5f);

        g_matWorldCube = matRotZ * matRotX * matRotY;
    }


    {// Update Texture
        static DWORD g_dwCount = 0;
        static DWORD g_dwTileColorR = 0;
        static DWORD g_dwTileColorG = 0;
        static DWORD g_dwTileColorB = 0;

        const DWORD TILE_WIDTH = 16;
        const DWORD TILE_HEIGHT = 16;

        DWORD TILE_WIDTH_COUNT = g_ImageWidth / TILE_WIDTH;
        DWORD TILE_HEIGHT_COUNT = g_ImageHeight / TILE_HEIGHT;

        if (g_dwCount >= TILE_WIDTH_COUNT * TILE_HEIGHT_COUNT)
        {
            g_dwCount = 0;
        }
        DWORD TileY = g_dwCount / TILE_WIDTH_COUNT;
        DWORD TileX = g_dwCount % TILE_WIDTH_COUNT;

        DWORD StartX = TileX * TILE_WIDTH;
        DWORD StartY = TileY * TILE_HEIGHT;


        //DWORD r = rand() % 256;
        //DWORD g = rand() % 256;
        //DWORD b = rand() % 256;

        DWORD r = g_dwTileColorR;
        DWORD g = g_dwTileColorG;
        DWORD b = g_dwTileColorB;


        DWORD* pDest = (DWORD*)g_pImage;
        for (DWORD y = 0; y < TILE_WIDTH; y++)
        {
            for (DWORD x = 0; x < TILE_WIDTH; x++)
            {
                if (StartX + x >= g_ImageWidth)
                    __debugbreak();

                if (StartY + y >= g_ImageHeight)
                    __debugbreak();

                pDest[(StartX + x) + (StartY + y) * g_ImageWidth] = 0xff000000 | (b << 16) | (g << 8) | r;
            }
        }
        g_dwCount++;
        g_dwTileColorR += 8;
        if (g_dwTileColorR > 255)
        {
            g_dwTileColorR = 0;
            g_dwTileColorG += 8;
        }
        if (g_dwTileColorG > 255)
        {
            g_dwTileColorG = 0;
            g_dwTileColorB += 8;
        }
        if (g_dwTileColorB > 255)
        {
            g_dwTileColorB = 0;
        }
        g_pRenderer->UpdateTextureWithImage(g_pDynamicTexHandle, g_pImage, g_ImageWidth, g_ImageHeight);
    }

    // update font
    int iTextWidth = 0;
    int iTextHeight = 0;
    WCHAR wchText[64] = {};
	DWORD dwTextLen = swprintf_s(wchText, L"Current FrameRate: %u", g_FPS);
    // 텍스트를 찍을 때 마다, RenderTarget을 만복사를 하는 것은 너무 느리다. 
    // 문자열 별로 Texture를 캐싱해서 사용해야 한다.
    if (wcscmp(g_wchText, wchText) != 0) {
		memset(g_pTextImage, 0, g_TextImageWidth * g_TextImageHeight * 4);
        g_pRenderer->WriteTextToBitmap(g_pTextImage, g_TextImageWidth, g_TextImageHeight, g_TextImageWidth * 4, &iTextWidth, &iTextHeight, g_pFontObj, wchText, dwTextLen);
		g_pRenderer->UpdateTextureWithImage(g_pTextTextureHandle, g_pTextImage, g_TextImageWidth, g_TextImageHeight);
		wcsncpy_s(g_wchText, wchText, 64);
    }
    
    UpdateGridPos();
}

void* CreateTileGrid()
{
    std::vector<ColorMeshData> meshData;
    meshData.push_back(ColorMeshData());

    // 간격이 너무 좁은것 같아서 넓혀주었다.
    int vertexCount = 11;
    g_GridCellOffset = 25;

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
    TextureMeshData mesh;

    std::vector<XMFLOAT3> posL;
    posL.resize(24);

    float w2 = 0.5f * _width;
    float h2 = 0.5f * _height;
    float d2 = 0.5f * _depth;

    mesh.Vertices.resize(24);

    // 앞면
    mesh.Vertices[0] = TextureVertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    mesh.Vertices[1] = TextureVertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    mesh.Vertices[2] = TextureVertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    mesh.Vertices[3] = TextureVertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    posL[0] = XMFLOAT3(-w2, -h2, -d2);
    posL[1] = XMFLOAT3(-w2, +h2, -d2);
    posL[2] = XMFLOAT3(+w2, +h2, -d2);
    posL[3] = XMFLOAT3(+w2, -h2, -d2);

    // 뒷면
    mesh.Vertices[4] = TextureVertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    mesh.Vertices[5] = TextureVertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    mesh.Vertices[6] = TextureVertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    mesh.Vertices[7] = TextureVertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    posL[4] = XMFLOAT3(-w2, -h2, +d2);
    posL[5] = XMFLOAT3(+w2, -h2, +d2);
    posL[6] = XMFLOAT3(+w2, +h2, +d2);
    posL[7] = XMFLOAT3(-w2, +h2, +d2);

    // 윗면
    mesh.Vertices[8] = TextureVertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    mesh.Vertices[9] = TextureVertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    mesh.Vertices[10] = TextureVertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    mesh.Vertices[11] = TextureVertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    posL[8] = XMFLOAT3(-w2, +h2, -d2);
    posL[9] = XMFLOAT3(-w2, +h2, +d2);
    posL[10] = XMFLOAT3(+w2, +h2, +d2);
    posL[11] = XMFLOAT3(+w2, +h2, -d2);

    // 밑면
    mesh.Vertices[12] = TextureVertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    mesh.Vertices[13] = TextureVertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    mesh.Vertices[14] = TextureVertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    mesh.Vertices[15] = TextureVertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    posL[12] = XMFLOAT3(-w2, -h2, -d2);
    posL[13] = XMFLOAT3(+w2, -h2, -d2);
    posL[14] = XMFLOAT3(+w2, -h2, +d2);
    posL[15] = XMFLOAT3(-w2, -h2, +d2);

    // 왼면
    mesh.Vertices[16] = TextureVertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    mesh.Vertices[17] = TextureVertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    mesh.Vertices[18] = TextureVertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    mesh.Vertices[19] = TextureVertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

    posL[16] = XMFLOAT3(-w2, -h2, +d2);
    posL[17] = XMFLOAT3(-w2, +h2, +d2);
    posL[18] = XMFLOAT3(-w2, +h2, -d2);
    posL[19] = XMFLOAT3(-w2, -h2, -d2);

    // 오른면
    mesh.Vertices[20] = TextureVertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    mesh.Vertices[21] = TextureVertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    mesh.Vertices[22] = TextureVertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    mesh.Vertices[23] = TextureVertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    posL[20] = XMFLOAT3(+w2, -h2, -d2);
    posL[21] = XMFLOAT3(+w2, +h2, -d2);
    posL[22] = XMFLOAT3(+w2, +h2, +d2);
    posL[23] = XMFLOAT3(+w2, -h2, +d2);

    mesh.Indices32.resize(36);

    mesh.Indices32[0] = 0; mesh.Indices32[1] = 1; mesh.Indices32[2] = 2;
    mesh.Indices32[3] = 0; mesh.Indices32[4] = 2; mesh.Indices32[5] = 3;

    mesh.Indices32[6] = 4; mesh.Indices32[7] = 5; mesh.Indices32[8] = 6;
    mesh.Indices32[9] = 4; mesh.Indices32[10] = 6; mesh.Indices32[11] = 7;

    mesh.Indices32[12] = 8; mesh.Indices32[13] = 9; mesh.Indices32[14] = 10;
    mesh.Indices32[15] = 8; mesh.Indices32[16] = 10; mesh.Indices32[17] = 11;

    mesh.Indices32[18] = 12; mesh.Indices32[19] = 13; mesh.Indices32[20] = 14;
    mesh.Indices32[21] = 12; mesh.Indices32[22] = 14; mesh.Indices32[23] = 15;

    mesh.Indices32[24] = 16; mesh.Indices32[25] = 17; mesh.Indices32[26] = 18;
    mesh.Indices32[27] = 16; mesh.Indices32[28] = 18; mesh.Indices32[29] = 19;

    mesh.Indices32[30] = 20; mesh.Indices32[31] = 21; mesh.Indices32[32] = 22;
    mesh.Indices32[33] = 20; mesh.Indices32[34] = 22; mesh.Indices32[35] = 23;

    std::vector<uint32_t> adjIndices;
    GenerateAdjacencyIndices(posL, mesh.Indices32, adjIndices);

    std::vector<SubmeshRange> ranges;
    ranges.reserve(6);
    for (UINT i = 0; i < 6; i++)
    {
        SubmeshRange range = {};
        range.startPosIndex = i * 4;
        range.posIndexCount = 4;
        range.startIndexIndex = i * 6;
        range.indexIndexCount = 6;
        ranges.push_back(range);
    }

    TextureRenderMesh* pNewCube = new TextureRenderMesh;
    pNewCube->Initialize(g_pRenderer);

    if (!pNewCube->CreateRenderAssetsFromSingleMesh(mesh, adjIndices, ranges))
    {
        delete pNewCube;
        return nullptr;
    }

    const WCHAR* saltTexFiles[6] =
    {
        L"../../Assets/salt_01.dds",
        L"../../Assets/salt_02.dds",
        L"../../Assets/salt_03.dds",
        L"../../Assets/salt_04.dds",
        L"../../Assets/salt_05.dds",
        L"../../Assets/salt_06.dds"
    };

    for (UINT i = 0; i < 6; i++)
    {
        if (!g_pCubeFaceTextures[i])
        {
            g_pCubeFaceTextures[i] = reinterpret_cast<TEXTURE_HANDLE*>(g_pRenderer->CreateTextureFromFile(saltTexFiles[i]));
        }

        if (g_pCubeFaceTextures[i])
        {
            pNewCube->BindTextureAssets(g_pCubeFaceTextures[i], i);
        }

        //CONSTANT_BUFFER_MATERIAL purpleMat = CONSTANT_BUFFER_MATERIAL(XMFLOAT4(0.5f, 0.2f, 0.7f, 1.f));
        CONSTANT_BUFFER_MATERIAL whiteMat = CONSTANT_BUFFER_MATERIAL(XMFLOAT4(1.f, 1.f, 1.f, 1.f));
        pNewCube->SetMaterial(whiteMat, i);
    }

    return pNewCube;
}


//CHAR wchTxt[64];
//sprintf_s(wchTxt, "Camera Pos (%f, %f)\n", curCameraPos.x, curCameraPos.z);
//OutputDebugStringA(wchTxt);

void UpdateGridPos()
{
    XMFLOAT3 curCameraPos = g_pRenderer->GetCameraWorldPos();

    float xOffset = curCameraPos.x / g_GridCellOffset;
    float zOffset = curCameraPos.z / g_GridCellOffset;
    
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
