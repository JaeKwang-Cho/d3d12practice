#include "pch.h"
#include "CommonAssets.h"
#include "D3D12Renderer.h"

TEXTURE_HANDLE* DEFAULT_WHITE_TEXTURE = nullptr;
void* DEFAULT_BOX = nullptr;

void* CreateDefaultBox(D3D12Renderer* _pRenderer);


void CreateCommonAssets(D3D12Renderer* _pRenderer) {
	// default texture
	if (!DEFAULT_WHITE_TEXTURE) {
		DEFAULT_WHITE_TEXTURE = (TEXTURE_HANDLE*)_pRenderer->CreateTextureFromFile(L"../../Assets/white.png");
	}
}

void DeleteCommonAssets(D3D12Renderer* _pRenderer)
{
	_pRenderer->DeleteTexture(DEFAULT_WHITE_TEXTURE);
}

void* CreateDefaultBox(D3D12Renderer* _pRenderer)
{
	return nullptr;
}