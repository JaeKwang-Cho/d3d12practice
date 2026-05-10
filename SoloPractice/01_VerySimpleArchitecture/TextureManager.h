#pragma once
#include <unordered_map>

class D3D12Renderer;
class D3D12ResourceManager

class TextureManager
{
public:
	bool Initalize(D3D12Renderer* _pRenderer);
	TEXTURE_HANDLE* CreateTextureFromFile_ITL(const WCHAR* _wchFileName);
	TEXTURE_HANDLE* CreateDynamicTexture_ITL(UINT _TexWidth, UINT _TexHeight);
	TEXTURE_HANDLE* CreateImmutableTexture(UINT _TexWidth, UINT _TexHeight, DXGI_FORMAT _format, const BYTE* _pInitImage);

	void DeleteTexture(TEXTURE_HANDLE* _pTexHandle);

	TextureManager();
	virtual ~TextureManager();
private:
	D3D12Renderer* m_pRenderer;
	D3D12ResourceManager* m_pResourceManager;

	std::unordered_map<std::wstring, TEXTURE_HANDLE*> m_pTextureHashTable;

	TEXTURE_HANDLE* AllocTextureHandle_ITL();
	void CleanUpTextureManager();
};

