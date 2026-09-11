#include "pch.h"
#include <Windows.h>
#include <DirectXMath.h>
#include "D3D12Renderer.h"
#include "GameObject.h"
#include "Game.h"
#include <filesystem>

using namespace DirectX;

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

	memset(m_KeyState, 0, sizeof(m_KeyState));

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
	CreateGameObjectAsBottom_ITL();

	return true;
}

void Game::OnKeyDown(UINT _nChar, UINT _uiScanCode)
{
	if (_nChar < 256)
	{
		m_KeyState[_nChar] = true;
	}

	switch (_nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = TRUE;
		break;
	case 'R':
	{
		bool bUseDXR = m_pRenderer->IsEnabledDXR();
		bUseDXR = bUseDXR == 0;
		m_pRenderer->EnableDXR(bUseDXR);
	}
	break;
	default:
		break;
	}
}

void Game::OnKeyUp(UINT _nChar, UINT _uiScanCode)
{
	if (_nChar < 256)
	{
		m_KeyState[_nChar] = false;
	}

	if (_nChar == VK_SHIFT)
	{
		m_bShiftKeyDown = FALSE;
	}
}

void Game::OnMouseLButtonDown(int _x, int _y, UINT _nFlags)
{
	m_bMouseLButtonDown = TRUE;
	m_bCamRotMode = TRUE;
	m_iCurMouseX = _x;
	m_iCurMouseY = _y;
	m_iPrvMouseX = _x;
	m_iPrvMouseY = _y;
}

void Game::OnMouseLButtonUp(int _x, int _y, UINT _nFlags)
{
	m_bMouseLButtonDown = FALSE;
	m_bCamRotMode = m_bMouseRButtonDown;
}

void Game::OnMouseRButtonDown(int _x, int _y, UINT _nFlags)
{
	m_bCamRotMode = TRUE;
	m_iMouseX_RButtonPressed = _x;
	m_iMouseY_RButtonPressed = _y;
	m_bMouseRButtonDown = TRUE;

	m_iCurMouseX = _x;
	m_iCurMouseY = _y;
	m_iPrvMouseX = _x;
	m_iPrvMouseY = _y;
}

void Game::OnMouseRButtonUp(int _x, int _y, UINT _nFlags)
{
	m_bMouseRButtonDown = FALSE;
	m_bCamRotMode = m_bMouseLButtonDown;
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

	const int dx = _x - m_iPrvMouseX;
	const int dy = _y - m_iPrvMouseY;

	if (m_bCamRotMode && (m_bMouseLButtonDown || m_bMouseRButtonDown))
	{
		// 반전 없이: 마우스 이동 방향 그대로 yaw/pitch 누적
		const float fYaw = static_cast<float>(dx) * m_fMouseSensitivity;
		const float fPitch = static_cast<float>(dy) * m_fMouseSensitivity;
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

void Game::OnRawMouseDelta(LONG _lDeltaX, LONG _lDeltaY)
{
}

void Game::OnFocusLost()
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
	if (m_PrvUpdateTick == 0)
	{
		m_PrvUpdateTick = _CurTick;
		return FALSE;
	}

	const ULONGLONG deltaMs = _CurTick - m_PrvUpdateTick;
	m_PrvUpdateTick = _CurTick;

	float deltaSec = static_cast<float>(deltaMs) * 0.001f;
	if (deltaSec <= 0.0f)
	{
		return FALSE;
	}
	if (deltaSec > 0.1f)
	{
		deltaSec = 0.1f;
	}

	// 카메라 이동 입력 (카메라 로컬 기준)
	// 요청 반영: W 전진, S 후진, A 오른쪽, D 왼쪽, Q 상승, E 하강
	float moveX = 0.0f; // right(+)
	float moveY = 0.0f; // up(+)
	float moveZ = 0.0f; // forward(+)

	if (m_KeyState['W']) moveZ += 1.0f;
	if (m_KeyState['S']) moveZ -= 1.0f;
	if (m_KeyState['A']) moveX -= 1.0f; // A = 왼쪽
	if (m_KeyState['D']) moveX += 1.0f; // D = 오른쪽쪽
	if (m_KeyState['Q']) moveY += 1.0f; // 상승
	if (m_KeyState['E']) moveY -= 1.0f; // 하강

	// 대각선 속도 보정
	const float lenSq = moveX * moveX + moveY * moveY + moveZ * moveZ;
	if (lenSq > 0.0f)
	{
		const float invLen = 1.0f / sqrtf(lenSq);
		moveX *= invLen;
		moveY *= invLen;
		moveZ *= invLen;

		const float step = m_fMoveSpeed * deltaSec;
		m_pRenderer->MoveCamera(moveX * step, moveY * step, moveZ * step);
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

void Game::BeginLookMode_ITL()
{
	if (m_bLookMode) return;
	m_bLookMode = true;

	GetCursorPos(&m_ptCursorRestore); // 현재 커서 위치 저장
	SetCapture(m_hWnd);
	static_assert(false && "Not implemented");
}

void Game::EndLookMode_ITL()
{
	
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
