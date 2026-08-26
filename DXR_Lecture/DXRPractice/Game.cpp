#include "pch.h"
#include <Windows.h>
#include <DirectXMath.h>
#include "D3D12Renderer.h"
#include "GameObject.h"
#include "Game.h"
#include <filesystem>

Game::Game()
{
}

Game::~Game()
{
	Cleanup_ITL();
}

bool Game::Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader)
{
	const ULONG BOX_OBJ_COUNT = 100;
	const ULONG GAME_OBJ_COUNT = BOX_OBJ_COUNT + 1; // box meshes + bottom

	WCHAR wchAppPath[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, wchAppPath);

	WCHAR wchExePath[_MAX_PATH];
	GetModuleFileNameW(nullptr, wchExePath, _MAX_PATH);

	std::filesystem::path shaderPath =
		std::filesystem::path(wchExePath).parent_path() / L"..\\Shaders";
	shaderPath = std::filesystem::canonical(shaderPath);

	SetCurrentDirectory(wchAppPath);

	m_pRenderer = std::make_unique<D3D12Renderer>();
	m_pRenderer->Initialize(_hWnd, _bEnableDebugLayer, _bEnableGBV, _bDebugShader, shaderPath.wstring().c_str(), GAME_OBJ_COUNT);
	m_hWnd = _hWnd;

	// Create Font
	m_pFontObj = m_pRenderer->CreateFontObject(L"Tahoma", 18.0f);
	
	// create texture for draw text
	m_TextImageWidth = 512;
	m_TextImageHeight = 256;
	m_pTextImage = (BYTE*)malloc(m_TextImageWidth * m_TextImageHeight * 4);
	m_pTextTexTexHandle = m_pRenderer->CreateDynamicTexture(m_TextImageWidth, m_TextImageHeight);
	memset(m_pTextImage, 0, m_TextImageWidth * m_TextImageHeight * 4);

	m_pSpriteObjCommon = m_pRenderer->CreateSpriteObject();

	for (ULONG i = 0; i < BOX_OBJ_COUNT; i++)
	{
		GameObject* pGameObj = CreateGameObjectAsBox_ITL();
		if (pGameObj)
		{
			float x = (float)((rand() % 21) - 10);	// -10m - 10m 
			float y = (float)((rand() % 13) - 2) * 0.1f;	// -20cm - 1m
			float z = (float)((rand() % 21) - 10);	// -10m - 10m 
			pGameObj->SetPosition(x, y, z);
			float rad = (rand() % 181) * (3.1415f / 180.0f);
			pGameObj->SetRotationY(rad);
		}
	}
	GameObject* pBottom = CreateGameObjectAsBottom_ITL();

	return TRUE;
}

GameObject* Game::CreateGameObjectAsBox_ITL()
{
	std::unique_ptr<GameObject> pGameObj = std::make_unique<GameObject>();
	pGameObj->Initialize(this);
	pGameObj->CreateBoxMeshObject();

	GameObject* pRawPtr = pGameObj.get();
	m_GameObjects.insert(std::make_pair(pRawPtr, std::move(pGameObj)));

	return pRawPtr;
}

GameObject* Game::CreateGameObjectAsBottom_ITL()
{
	std::unique_ptr<GameObject> pGameObj = std::make_unique<GameObject>();
	pGameObj->Initialize(this);
	pGameObj->CreateBottomMeshObject();

	GameObject* pRawPtr = pGameObj.get();
	m_GameObjects.insert(std::make_pair(pRawPtr, std::move(pGameObj)));

	return pRawPtr;
}

void Game::OnKeyDown(UINT _nChar, UINT _uiScanCode)
{
	switch (_nChar)
	{
		case VK_SHIFT:
			m_bShiftKeyDown = TRUE;
			break;
		case 'W':
			if (m_bShiftKeyDown)
			{
				m_CamOffsetY = 0.05f;
			}
			else
			{
				m_CamOffsetZ = 0.05f;
			}
			break;
		case 'S':
			if (m_bShiftKeyDown)
			{
				m_CamOffsetY = -0.05f;
			}
			else
			{
				m_CamOffsetZ = -0.05f;
			}
			break;
		case 'A':
			m_CamOffsetX = -0.05f;
			break;
		case 'D':
			m_CamOffsetX = 0.05f;
			break;
		case 'R':
			{
				bool bUseDXR = m_pRenderer->IsEnabledDXR();
				bUseDXR = bUseDXR == 0;
				m_pRenderer->EnableDXR(bUseDXR);
			}
			break;
	}
}

void Game::OnKeyUp(UINT _nChar, UINT _uiScanCode)
{
	switch (_nChar)
	{
		case VK_SHIFT:
			m_bShiftKeyDown = FALSE;
			break;
		case 'W':
			m_CamOffsetY = 0.0f;
			m_CamOffsetZ = 0.0f;
			break;
		case 'S':
			m_CamOffsetY = 0.0f;
			m_CamOffsetZ = 0.0f;
			break;
		case 'A':
			m_CamOffsetX = 0.0f;
			break;
		case 'D':
			m_CamOffsetX = 0.0f;
			break;
	}
}

void Game::OnMouseLButtonDown(int _x, int _y, UINT _nFlags)
{
	m_bMouseLButtonDown = TRUE;
}

void Game::OnMouseLButtonUp(int _x, int _y, UINT _nFlags)
{
	m_bMouseLButtonDown = FALSE;
}

void Game::OnMouseRButtonDown(int _x, int _y, UINT _nFlags)
{
	m_bCamRotMode = TRUE;
	m_iMouseX_RButtonPressed = _x;
	m_iMouseY_RButtonPressed = _y;

	m_bMouseRButtonDown = TRUE;
}

void Game::OnMouseRButtonUp(int _x, int _y, UINT _nFlags)
{
	m_bCamRotMode = FALSE;
	m_bMouseRButtonDown = FALSE;	
}

void Game::OnMouseMButtonDown(int _x, int _y, UINT _nFlags)
{
	m_bMouseMButtonDown = TRUE;
}

void Game::OnMouseMButtonUp(int _x, int _y, UINT _nFlags)
{
	m_bMouseMButtonDown = FALSE;
}

void Game::OnMouseMove(int _x, int _y, UINT _nFlags)
{
	m_iPrvMouseX = m_iCurMouseX;
	m_iPrvMouseY = m_iCurMouseY;

	int dx = _x - m_iPrvMouseX;
	int dy = _y - m_iPrvMouseY;

	if (m_bCamRotMode)
	{
		if (dy != 0)
			int a = 0;

		float fYaw = (float)dx * 0.01f;
		float fPitch = (float)dy * 0.01f;
		m_pRenderer->ApplyCameraRot(fYaw, fPitch, 0.0f);
	}
	m_iCurMouseX = _x;
	m_iCurMouseY = _y;
}

void Game::OnMouseWheel(int _x, int _y, int _iWheel)
{
}

void Game::OnMouseHWheel(int _x, int _y, int _iWheel)
{
}

void Game::Run()
{
	m_FrameCount++;

	ULONGLONG CurTick = GetTickCount64();

	Update(CurTick);

	Render_ITL();

	if (CurTick - m_PrvFrameCheckTick > 1000)
	{
		m_PrvFrameCheckTick = CurTick;	
				
		WCHAR wchTxt[64];
		m_FPS = m_FrameCount;
		swprintf_s(wchTxt, L"FPS:%u", m_FPS);
		SetWindowText(m_hWnd, wchTxt);
				
		m_FrameCount = 0;
	}
}

bool Game::Update(ULONGLONG _CurTick)
{	
	// Update Scene with 60FPS
	if (_CurTick - m_PrvUpdateTick < 16)
	{
		return FALSE;
	}
	m_PrvUpdateTick = _CurTick;

	// Update camera
	if (m_CamOffsetX != 0.0f || m_CamOffsetY != 0.0f || m_CamOffsetZ != 0.0f)
	{
		m_pRenderer->MoveCamera(m_CamOffsetX, m_CamOffsetY, m_CamOffsetZ);
	}
	
	// update game objects
	for (const auto& pair : m_GameObjects)
	{
		GameObject* pGameObj = pair.first;
		pGameObj->Run();
	}
	
	// update status text
	int iTextWidth = 0;
	int iTextHeight = 0;
	WCHAR wchTxt[64] = {};
	ULONG ulTxtLen = swprintf_s(wchTxt, L"Current FrameRate: %u", m_FPS);

	if (wcscmp(m_wchText, wchTxt))
	{
		memset(m_pTextImage, 0, m_TextImageWidth * m_TextImageHeight * 4);
		m_pRenderer->WriteTextToBitmap(m_pTextImage, m_TextImageWidth, m_TextImageHeight, m_TextImageWidth * 4, &iTextWidth, &iTextHeight, m_pFontObj, wchTxt, ulTxtLen);
		m_pRenderer->UpdateTextureWithImage(m_pTextTexTexHandle, m_pTextImage, m_TextImageWidth, m_TextImageHeight);
		wcscpy_s(m_wchText, wchTxt);
	}
	else
	{
		int a = 0;
	}
	return TRUE;
}

void Game::Render_ITL()
{
	m_pRenderer->BeginRender();

	// render game objects
	ULONG ulObjCount = 0;
	for (const auto& pair : m_GameObjects)
	{
		GameObject* pGameObj = pair.first;
		pGameObj->Render();
		ulObjCount++;
	}	
	// render dynamic texture as text
	m_pRenderer->RenderSpriteWithTex(m_pSpriteObjCommon, 512 + 5, 256 + 5 + 256 + 5, 1.0f, 1.0f, nullptr, 0.0f, m_pTextTexTexHandle);

	m_pRenderer->EndRender();
	m_pRenderer->Present();
}

void Game::DeleteGameObject_ITL(GameObject* _pGameObj)
{
	auto iter = m_GameObjects.find(_pGameObj);
	if (iter != m_GameObjects.end())
	{
		m_GameObjects.erase(iter);
	}
}

void Game::DeleteAllGameObjects_ITL()
{
	m_GameObjects.clear();
}

bool Game::UpdateWindowSize(ULONG _dwBackBufferWidth, ULONG _dwBackBufferHeight)
{
	bool bResult = FALSE;
	if (m_pRenderer)
	{
		bResult = m_pRenderer->UpdateWindowSize(_dwBackBufferWidth, _dwBackBufferHeight);
	}
	return bResult;
}

void Game::Cleanup_ITL()
{
	DeleteAllGameObjects_ITL();

	if (m_pTextImage)
	{
		free(m_pTextImage);
		m_pTextImage = nullptr;
	}
	if (m_pRenderer)
	{
		if (m_pFontObj)
		{
			m_pRenderer->DeleteFontObject(m_pFontObj);
			m_pFontObj = nullptr;
		}
	
		if (m_pTextTexTexHandle)
		{
			m_pRenderer->DeleteTexture(m_pTextTexTexHandle);
			m_pTextTexTexHandle = nullptr;
		}
		if (m_pSpriteObjCommon)
		{
			m_pRenderer->DeleteSpriteObject(m_pSpriteObjCommon);
			m_pSpriteObjCommon = nullptr;
		}

		m_pRenderer.reset();
	}
}
