#pragma once
#include <map>

class D3D12Renderer;
class GameObject;
class GameTimer;

class Game
{
public:
	bool Initialize_Game(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV);
	void Run();
	bool Update(ULONGLONG _CurTick);
	
	bool UpdateWindowSize(ULONG _dwBackBufferWidth, ULONG _dwBackBufferHeight);
	void OnRButtonDown(WPARAM _btnState, int _x, int _y);
	void OnRButtonUp(WPARAM _btnState, int _x, int _y);
	void OnMouseMove(WPARAM _btnState, int _x, int _y);
	void OnKeyboardInput();

	D3D12Renderer* INL_GetRenderer() { return m_pRenderer.get(); }

	Game();
	virtual ~Game();

private:
	void Render();
	GameObject* CreateGameObject();
	void DeleteGameObject(GameObject* _pGameObj);
	void DeleteAllGameObjects();

	void Cleanup_Game();

private:
	std::unique_ptr<D3D12Renderer> m_pRenderer;
	HWND m_hWnd;
	void* m_pSpriteObjCommon;
 
	BYTE* m_pTextImage;
	UINT m_TextImageWidth;
	UINT m_TextImageHeight;
	void* m_pTextTextureHandle;
	std::unique_ptr<FONT_HANDLE> m_pFontObj;

	bool m_bShiftKeyDown;

	float m_CamOffsetX;
	float m_CamOffsetY;
	float m_CamOffsetZ;

	GameTimer m_GameTimer;

	std::map<GameObject*, std::unique_ptr<GameObject>> m_GameObjects;

	ULONGLONG m_PrevFrameCheckTick;
	ULONGLONG m_PrevUpdateTick;
	ULONG m_FrameCount;
	ULONG m_FPS;
	WCHAR m_wchText[64];
};

