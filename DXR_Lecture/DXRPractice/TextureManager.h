#pragma once
#include <map>

class D3D12Renderer;
class D3D12ResourceManager;

class TextureManager
{
public:
	bool Initialize(D3D12Renderer* _pRenderer);
	TEXTURE_HANDLE* CreateTextureFromFile_ITL(const WCHAR* _wchFileName);
	TEXTURE_HANDLE* CreateDynamicTexture_ITL(UINT _TexWidth, UINT _TexHeight);
	TEXTURE_HANDLE* CreateImmutableTexture_ITL(UINT _TexWidth, UINT _TexHeight, DXGI_FORMAT _format, const BYTE* _pInitImage);

	void DeleteTexture_ITL(TEXTURE_HANDLE* _pTexHandle);

	TextureManager();
	virtual ~TextureManager();
private:
	D3D12Renderer* m_pRenderer;
	D3D12ResourceManager* m_pResourceManager;

	std::map<std::wstring, std::unique_ptr<TEXTURE_HANDLE>> m_TextureHashTable;
	std::map<TEXTURE_HANDLE*, std::wstring> m_TextureReverseHashTable;
	std::map<TEXTURE_HANDLE*, std::unique_ptr<TEXTURE_HANDLE>> m_TextureHashSet;

	TEXTURE_HANDLE* AllocTextureHandle_ITL();
	void CleanUpTextureManager();
};

