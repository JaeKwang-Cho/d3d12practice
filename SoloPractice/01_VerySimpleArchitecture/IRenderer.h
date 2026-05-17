#pragma once
#include "IRenderer_typedef.h"
#include <memory>

class GameTimer;
class IRenderMesh;

class IRenderer
{
public:
	virtual ~IRenderer() = default;

	virtual void SetAssetRootPath(const WCHAR* _wchAssetRootPath) = 0;
	virtual void SetShaderRootPath(const WCHAR* _wchShaderRootPath) = 0;

	virtual bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV) = 0;

	virtual void Update(const GameTimer& _gameTimer) = 0;
	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;
	virtual void Present() = 0;
	virtual void FlushMultiRendering() = 0;

	virtual bool UpdateWindowSize_Renderer(DWORD _dwWidth, DWORD _dwHeight) = 0;

	virtual IRenderMesh* CreateTextureRenderMesh(
		const TextureMeshData& _mesh,
		const std::vector<std::uint32_t>& _adjIndices,
		const std::vector<SubmeshRange>& _ranges) = 0;
	virtual void DeleteRenderMesh(IRenderMesh* _pMeshObjectHandle) = 0;
	virtual void BindTextureToMesh(IRenderMesh* _pMeshObjectHandle, TEXTURE_HANDLE* _pTexHandle, UINT _subRenderAssetIndex) = 0;
	virtual void SetMeshMaterial(IRenderMesh* _pMeshObjectHandle, const CONSTANT_BUFFER_MATERIAL& _MaterialData, UINT _subRenderAssetIndex) = 0;
	virtual void DrawRenderMesh(IRenderMesh* _pMeshObjectHandle, const DirectX::XMMATRIX* _pMatWorld) = 0;
	virtual void DrawOutlineMesh(IRenderMesh* _pMeshObjectHandle, const DirectX::XMMATRIX* _pMatWorld) = 0;

	virtual SPRITE_HANDLE* CreateSpriteObject() = 0;
	virtual SPRITE_HANDLE* CreateSpriteObject(const WCHAR* _wchTexFileName, int _posX, int _posY, int _width, int _height) = 0;
	virtual void DeleteSpriteObject(SPRITE_HANDLE* _pSpriteObjHandle) = 0;
	virtual void RenderSpriteWithTex(SPRITE_HANDLE* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, const RECT* _pRect, float _z, TEXTURE_HANDLE* _pTexHandle) = 0;
	virtual void RenderSprite(SPRITE_HANDLE* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, float _z) = 0;

	virtual TEXTURE_HANDLE* CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b) = 0;
	virtual TEXTURE_HANDLE* CreateTextureFromFile(const WCHAR* _wchFileName) = 0;
	virtual TEXTURE_HANDLE* CreateDynamicTexture(UINT _TexWidth, UINT _TexHeight) = 0;
	virtual void UpdateTextureWithImage(TEXTURE_HANDLE* _pTexHandle, const BYTE* _pSrcBytes, UINT _SrcWidth, UINT _SrcHeight) = 0;
	virtual void DeleteTexture(TEXTURE_HANDLE* _pTexHandle) = 0;

	virtual FONT_HANDLE* CreateFontObject(const WCHAR* _wchFontFamilyName, float _fFontSize) = 0;
	virtual void DeleteFontObject(FONT_HANDLE* _pFontHandle) = 0;
	virtual bool WriteTextToBitmap(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, FONT_HANDLE* _pFontHandle, const WCHAR* _wchString, DWORD _dwLen) = 0;

	virtual void DrawGrid() = 0;
	virtual void UpdateGridWorldMatrix(UINT _gridCellOffset = 25) = 0;

	virtual void OnRButtonDown_Renderer(WPARAM _btnState, int _x, int _y) = 0;
	virtual void OnRButtonUp_Renderer(WPARAM _btnState, int _x, int _y) = 0;
	virtual void OnMouseMove_Renderer(WPARAM _btnState, int _x, int _y) = 0;
	virtual void OnKeyboardInput_Renderer(const GameTimer& _gameTimer) = 0;
};

std::unique_ptr<IRenderer> CreateRenderer();

