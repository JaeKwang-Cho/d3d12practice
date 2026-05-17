#pragma once
#include <map>
#include "IRenderer.h"

class D3D12Renderer;
class GameObject;
class GameTimer;

class Game
{
public:
	bool Initialize_Game(HWND _hWnd, std::unique_ptr<IRenderer> _pRenderer, bool _bEnableDebugLayer = false, bool _bEnableGBV = false);
	void Run();
	bool Update(ULONGLONG _CurTick);
	
	bool UpdateWindowSize(ULONG _dwBackBufferWidth, ULONG _dwBackBufferHeight);
	void OnRButtonDown(WPARAM _btnState, int _x, int _y);
	void OnRButtonUp(WPARAM _btnState, int _x, int _y);
	void OnMouseMove(WPARAM _btnState, int _x, int _y);
	void OnKeyboardInput();

	IRenderer* INL_GetRenderer() { return m_pRenderer.get(); }

	Game();
	virtual ~Game();

private:
	void Render();
	GameObject* CreateGameObject();
	void DeleteGameObject(GameObject* _pGameObj);
	void DeleteAllGameObjects();

	void Cleanup_Game();

private:
	std::unique_ptr<IRenderer> m_pRenderer;
	HWND m_hWnd;
	SPRITE_HANDLE* m_pSpriteObjCommon;
 
	BYTE* m_pTextImage;
	UINT m_TextImageWidth;
	UINT m_TextImageHeight;
	TEXTURE_HANDLE* m_pTextTextureHandle;
	FONT_HANDLE* m_pFontObj;

	GameTimer m_GameTimer;

	std::map<GameObject*, std::unique_ptr<GameObject>> m_GameObjects;

	ULONGLONG m_PrevFrameCheckTick;
	ULONGLONG m_PrevUpdateTick;
	ULONG m_FrameCount;
	ULONG m_FPS;
	WCHAR m_wchText[64];
};

