#pragma once
#include <map>
#include <memory>

class D3D12Renderer;
class GameObject;

class Game
{
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader);
	void Run();
	bool Update(ULONGLONG _CurTick);
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
	bool UpdateWindowSize(ULONG _dwBackBufferWidth, ULONG _dwBackBufferHeight);

	D3D12Renderer* INL_GetRenderer() const { return m_pRenderer.get(); }

	Game();
	virtual ~Game();

private:
	void Render_ITL();
	GameObject* CreateGameObjectAsBox_ITL();
	GameObject* CreateGameObjectAsBottom_ITL();
	void DeleteGameObject_ITL(GameObject* _pGameObj);
	void DeleteAllGameObjects_ITL();
	void Cleanup_ITL();

private:
	std::unique_ptr<D3D12Renderer> m_pRenderer = nullptr;
	HWND m_hWnd = nullptr;
	void* m_pSpriteObjCommon = nullptr;

	BYTE* m_pTextImage = nullptr;
	UINT m_TextImageWidth = 0;
	UINT m_TextImageHeight = 0;
	void* m_pTextTexTexHandle = nullptr;
	void* m_pFontObj = nullptr;

	bool m_bShiftKeyDown = FALSE;

	float m_CamOffsetX = 0.0f;
	float m_CamOffsetY = 0.0f;
	float m_CamOffsetZ = 0.0f;

	bool m_bCamRotMode = FALSE;
	int m_iCurMouseX = 0;
	int m_iCurMouseY = 0;
	int m_iPrvMouseX = 0;
	int m_iPrvMouseY = 0;
	int m_iMouseX_RButtonPressed = 0;
	int m_iMouseY_RButtonPressed = 0;
	bool m_bMouseLButtonDown = FALSE;
	bool m_bMouseMButtonDown = FALSE;
	bool m_bMouseRButtonDown = FALSE;

	std::map<GameObject*, std::unique_ptr<GameObject>> m_GameObjects;

	ULONGLONG m_PrvFrameCheckTick = 0;
	ULONGLONG m_PrvUpdateTick = 0;
	ULONG m_FrameCount = 0;
	ULONG m_FPS = 0;
	WCHAR m_wchText[64] = {};
};

