#include "pch.h"
#include "Game.h"
#include "D3D12Renderer.h"
#include "GameObject.h"
#include "CommonAssets.h"


bool Game::Initialize_Game(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV)
{
	m_pRenderer = std::make_unique<D3D12Renderer>();
	m_pRenderer->Initialize(_hWnd, _bEnableDebugLayer, _bEnableGBV);
	CreateCommonAssets(m_pRenderer.get());
	m_hWnd = _hWnd;

	// Create Font
	m_pFontObj = m_pRenderer->CreateFontObject(L"Consolas", 32.f);

	// Create Sprite Object for Text Rendering
	m_TextImageWidth = 512;
	m_TextImageHeight = 256;
	m_pTextImage = new BYTE[m_TextImageWidth * m_TextImageHeight * 4];

	m_pTextTextureHandle = m_pRenderer->CreateDynamicTexture(m_TextImageWidth, m_TextImageHeight);
	memset(m_pTextImage, 0, m_TextImageWidth * m_TextImageHeight * 4);

	m_pSpriteObjCommon = m_pRenderer->CreateSpriteObject();

	{// Test - Create Game Objects
		const ULONG GAME_OBJ_COUNT = 100;

		for (DWORD i = 0; i < GAME_OBJ_COUNT; i++)
		{
			GameObject* pGameObj = CreateGameObject();
			if (pGameObj)
			{
				float x = (float)((rand() % 13) - 7);	// -7m - 5m 
				float y = 0.0f;
				float z = (float)((rand() % 13) - 7);	// -3m - 3m 
				pGameObj->SetGameObjectPosition(x, y, z);
				float rad = (rand() % 181) * (3.1415f / 180.0f);
				pGameObj->SetGameObjectRotationY(rad);
			}
		}
	}

	m_GameTimer.Reset();
	m_GameTimer.Start();

	return true;
}

void Game::Run()
{
	m_FrameCount++;

	ULONGLONG CurTick = GetTickCount64();

	Update(CurTick);

	Render();

	if(CurTick - m_PrevFrameCheckTick >= 1000)
	{
		m_PrevFrameCheckTick = CurTick;

		WCHAR wchText[64];
		m_FPS = m_FrameCount;
		swprintf_s(wchText, L"Current FrameRate: %u", m_FPS);
		SetWindowText(m_hWnd, wchText);

		m_FrameCount = 0;
	}
}

bool Game::Update(ULONGLONG _CurTick)
{
	// Update 60fps
	if (_CurTick - m_PrevUpdateTick < 16) {
		return false;
	}
	m_PrevUpdateTick = _CurTick;

	// Camera Update
	m_GameTimer.Tick();
	m_pRenderer->Update(m_GameTimer);
	m_pRenderer->UpdateGridWorldMatrix();

	// Update Game Objects
	for (auto& pair : m_GameObjects)
	{
		GameObject* pGameObj = pair.first;
		pGameObj->Run();
	}

	// Update Status Text
	int iTextWidth = 0;
	int iTextHeight = 0;
	WCHAR wchText[64] = {};
	DWORD dwTextLen = swprintf_s(wchText, L"Current FrameRate: %u", m_FPS);

	if (wcscmp(m_wchText, wchText) != 0) {
		memset(m_pTextImage, 0, m_TextImageWidth * m_TextImageHeight * 4);
		m_pRenderer->WriteTextToBitmap(m_pTextImage, m_TextImageWidth, m_TextImageHeight, m_TextImageWidth * 4, &iTextWidth, &iTextHeight, m_pFontObj.get(), wchText, dwTextLen);
		m_pRenderer->UpdateTextureWithImage(m_pTextTextureHandle, m_pTextImage, m_TextImageWidth, m_TextImageHeight);
		wcsncpy_s(m_wchText, wchText, dwTextLen);
	}

	return true;
}

bool Game::UpdateWindowSize(ULONG _dwBackBufferWidth, ULONG _dwBackBufferHeight)
{
	if (!m_pRenderer) {
		__debugbreak();
		return false;
	}
	return m_pRenderer->UpdateWindowSize_Renderer(_dwBackBufferWidth, _dwBackBufferHeight);
}

void Game::OnRButtonDown(WPARAM _btnState, int _x, int _y)
{
	m_pRenderer->OnRButtonDown_Renderer(_btnState, _x, _y);
}

void Game::OnRButtonUp(WPARAM _btnState, int _x, int _y)
{
	m_pRenderer->OnRButtonUp_Renderer(_btnState, _x, _y);
}

void Game::OnMouseMove(WPARAM _btnState, int _x, int _y)
{
	m_pRenderer->OnMouseMove_Renderer(_btnState, _x, _y);
}

void Game::OnKeyboardInput()
{
	m_pRenderer->OnKeyboardInput_Renderer(m_GameTimer);
}

void Game::Render()
{
	m_pRenderer->BeginRender();
	// Render Game Objects
	for (auto& pair : m_GameObjects)
	{
		GameObject* pGameObj = pair.first;
		pGameObj->Render();
	}
	// Render Text
	m_pRenderer->RenderSpriteWithTex(m_pSpriteObjCommon, 10, 10, 1.0f, 1.0f, nullptr, 0.0f, m_pTextTextureHandle);

	// Draw Grid
	m_pRenderer->DrawGrid();

	m_pRenderer->EndRender();
	m_pRenderer->Present();
}

GameObject* Game::CreateGameObject()
{
	std::unique_ptr<GameObject> pGameObj = std::make_unique<GameObject>();
	pGameObj->Initialize_GameObject(this);

	GameObject* pGameObjRawPtr = pGameObj.get();
	m_GameObjects.insert({ pGameObjRawPtr, std::move(pGameObj) });

	return pGameObjRawPtr;
}

void Game::DeleteGameObject(GameObject* _pGameObj)
{
	auto iter = m_GameObjects.find(_pGameObj);
	if (iter != m_GameObjects.end())
	{
		m_GameObjects.erase(iter);
	}
	else
	{
		__debugbreak();
	}
}

void Game::DeleteAllGameObjects()
{
	m_GameObjects.clear();
}

void Game::Cleanup_Game()
{
	m_pRenderer->FlushMultiRendering();

	DeleteAllGameObjects();

	if (m_pRenderer) {
		if (m_pSpriteObjCommon) {
			m_pRenderer->DeleteSpriteObject(m_pSpriteObjCommon);
			m_pSpriteObjCommon = nullptr;
		}

		if (m_pTextTextureHandle) {
			m_pRenderer->DeleteTexture(m_pTextTextureHandle);
			m_pTextTextureHandle = nullptr;
		}

		DeleteCommonAssets(m_pRenderer.get());
	}

	m_pFontObj.reset();

	if (m_pTextImage) {
		delete[] m_pTextImage;
		m_pTextImage = nullptr;
	}
}

Game::Game() :
	m_pRenderer(nullptr), m_hWnd(nullptr), m_pSpriteObjCommon(nullptr),
	m_pTextImage(nullptr), m_TextImageWidth(0), m_TextImageHeight(0), m_pTextTextureHandle(nullptr), m_pFontObj(nullptr),
	m_PrevFrameCheckTick(0), m_PrevUpdateTick(0), m_FrameCount(0), m_FPS(0), m_GameTimer(), m_wchText{}
{
}

Game::~Game()
{
	Cleanup_Game();
}