// D3D12Renderer.h from "megayuchi"
// 괜히 최신 d3d12 사용해보기
#pragma once

#include <unordered_set>
#include "RenderThread.h"
#include "IRenderer.h"

class D3D12ResourceManager;
class ConstantBufferPool;
class DescriptorPool;
class SingleDescriptorAllocator;
class ConstantBufferManager;
class D3D12PSOCache;
class FlyCamera;
class ScreenCapturer;
class FontManager;
class TextureManager;
class RenderQueue;
class IRenderMesh;
class CommandListPool;
struct RENDER_ITEM;

#define USE_MULTI_THREAD_RENDERING (1)

class D3D12Renderer : public IRenderer
{
public:
	static const UINT MAX_DRAW_COUNT_PER_FRAME = 9192; // 한 프레임당 하나의 모델에 대해서 최대 그려질 횟수를 지정한다.
	static const UINT MAX_DESCRIPRTOR_COUNT = 9192; // Shader Resource View로서 Bind될 친구들의 최대 개수를 지정한다.
	static const UINT MAX_RENDER_THREAD_COUNT = 8;
public:
	virtual void SetAssetRootPath(const WCHAR* _wchAssetRootPath) override;
	virtual void SetShaderRootPath(const WCHAR* _wchShaderRootPath) override;

	virtual bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV) override;

	virtual void Update(const GameTimer& _gameTimer) override;
	virtual void BeginRender() override;
	virtual void EndRender() override;
	virtual void Present() override;

	virtual bool UpdateWindowSize_Renderer(DWORD _dwWidth, DWORD _dwHeight) override;

	// Mesh
	virtual IRenderMesh* CreateTextureRenderMesh(
		const TextureMeshData& _mesh,
		const std::vector<std::uint32_t>& _adjIndices,
		const std::vector<SubmeshRange>& _ranges) override;
	virtual void DeleteRenderMesh(IRenderMesh* _pMeshObjectHandle) override;
	virtual void BindTextureToMesh(IRenderMesh* _pMeshObjectHandle, TEXTURE_HANDLE* _pTexHandle, UINT _subRenderAssetIndex) override;
	virtual void SetMeshMaterial(IRenderMesh* _pMeshObjectHandle, const CONSTANT_BUFFER_MATERIAL& _MaterialData, UINT _subRenderAssetIndex) override;
	virtual void DrawRenderMesh(IRenderMesh* _pMeshObjectHandle, const DirectX::XMMATRIX* _pMatWorld) override;
	virtual void DrawOutlineMesh(IRenderMesh* _pMeshObjectHandle, const DirectX::XMMATRIX* _pMatWorld) override;

	// Sprite
	virtual SPRITE_HANDLE* CreateSpriteObject() override;
	virtual SPRITE_HANDLE* CreateSpriteObject(const WCHAR* _wchTexFileName, int _posX, int _posY, int _width, int _height) ;
	virtual void DeleteSpriteObject(SPRITE_HANDLE* _pSpriteObjHandle) override;
	virtual void RenderSpriteWithTex(SPRITE_HANDLE* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, const RECT* _pRect, float _z, TEXTURE_HANDLE* _pTexHandle) override;
	virtual void RenderSprite(SPRITE_HANDLE* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, float _z) override;

	// Texture
	virtual TEXTURE_HANDLE* CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b) override;
	virtual TEXTURE_HANDLE* CreateTextureFromFile(const WCHAR* _wchFileName) override;
	virtual TEXTURE_HANDLE* CreateDynamicTexture(UINT _TexWidth, UINT _TexHeight) override;
	virtual void UpdateTextureWithImage(TEXTURE_HANDLE* _pTexHandle, const BYTE* _pSrcBytes, UINT _SrcWidth, UINT _SrcHeight) override;
	virtual void DeleteTexture(TEXTURE_HANDLE* _pTexHandle) override;

	// Font
	virtual FONT_HANDLE* CreateFontObject(const WCHAR* _wchFontFamilyName, float _fFontSize) override;
	virtual void DeleteFontObject(FONT_HANDLE* _pFontHandle) override;
	virtual bool WriteTextToBitmap(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, FONT_HANDLE* _pFontHandle, const WCHAR* _wchString, DWORD _dwLen) override;

	// Input
	virtual void OnRButtonDown_Renderer(WPARAM _btnState, int _x, int _y) override;
	virtual void OnRButtonUp_Renderer(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseMove_Renderer(WPARAM _btnState, int _x, int _y) override;
	virtual void OnKeyboardInput_Renderer(const GameTimer& _gameTimer) override;

	// Grid
	virtual void DrawGrid() override;
	virtual void UpdateGridWorldMatrix(UINT _gridCellOffset = 25) override;

public:
	std::wstring ResolveAssetPath(const WCHAR* _wchPath) const;
	std::wstring ResolveShaderPath(const WCHAR* _wchPath) const;

	// PSO
	D3D12PipelineState_raw GetPSO(std::string _strPSOName);
	bool CachePSO(std::string _strPSOName, D3D12PipelineState_raw _pPSODesc);

	ULONG GetCommandListCount();
	// For Multi-Threaded Rendering
	void ProcessByThread(ULONG _ulThreadIndex);

	void FlushMultiRendering();

protected:
private:
	bool CreateDescriptorHeapForRTV();
	bool CreateDescriptorHeapForDSV();
	bool CreateDepthStencil(UINT _width, UINT _height);

	void InitCamera();

	void InitFrameCB();
	void UpdateFrameCB();

	void CreateFence();
	void CleanupFence();

	UINT64 DoFence();
	void WaitForFenceValue(UINT64 _expectedFenceValue);

	void CleanUpRenderer();

	void ReleaseAllTextureHandles();

	// For Multi-Threaded Rendering
	bool InitRenderThreadPool();
	void CleanUpRenderThreadPool();
	void AddItemToRenderQueue(const RENDER_ITEM& _RenderItem);

public:

protected:
private:
	HWND m_hWnd;
	D3D12Device_ptr m_pD3DDevice; // 나중에 다 typedef로 바꾸기
	D3D12CommandQueue_ptr m_pCommandQueue;
	
	// For Multi-Threaded Rendering
	std::unique_ptr<CommandListPool> m_ppCommandListPool[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT];
	std::unique_ptr<DescriptorPool> m_ppDescriptorPool[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT];
	// 얘도 Render Pipeline에 bind되어서 쓰이는 애들이다. CommandList만 분리해서는 절대 안된다.
	std::unique_ptr<ConstantBufferManager> m_ppConstantBufferManager[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT]; // 이제 pool에서 바로 빼오는 것이 아니라 manager를 통해서 가져온다.
	std::unique_ptr<RenderQueue> m_pRenderQueue[MAX_RENDER_THREAD_COUNT];// Render Queue
	RENDER_THREAD_DESC m_RenderThreadDescs[MAX_RENDER_THREAD_COUNT];
	ULONG m_ulRenderThreadCount;
	ULONG m_ulCurThreadIndex;

	// Frame 별 한번씩 넘어가는 CBV이다.
	D3D12Resource_ptr m_ppFrameUploadCBs[MAX_PENDING_FRAME_COUNT];
	void* m_ppFrameSystemMemAddrs[MAX_PENDING_FRAME_COUNT];

	// Resource를 GPU에 올려주는 친구
	std::unique_ptr<D3D12ResourceManager> m_pResourceManager;
	// CBV pool이랑 DescriptorPool 도 CommandList 마다 하나씩 만든다.

	
	// Descriptor(View)를 모아서 관리해주는 친구
	std::unique_ptr<SingleDescriptorAllocator>	m_pSingleDescriptorAllocator;
	// PSO를 캐싱해주는 친구
	std::unique_ptr<D3D12PSOCache> m_pD3D12PSOCache;
	// Font Manager
	std::unique_ptr<FontManager> m_pFontManager;
	// Texture Manager
	std::unique_ptr<TextureManager> m_pTextureManager;


	UINT64 m_ui64FenceValue;
	// CommandList 마다 기다리기를 바라는 Fence Value를 저장한다.
	UINT64 m_pui64LastFenceValue[MAX_PENDING_FRAME_COUNT];

	D3D_FEATURE_LEVEL m_FeatureLevel;
	DXGI_ADAPTER_DESC3 m_AdaptorDesc;

	DXGISwapChain_ptr m_pSwapChain;
	D3D12Resource_ptr m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT];
	D3D12Resource_ptr m_pDepthStencil;
	D3D12DescriptorHeap_ptr m_pRTVHeap;
	D3D12DescriptorHeap_ptr m_pDSVHeap;
	D3D12DescriptorHeap_ptr m_pSRVHeap;

	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;
	UINT m_dsvDescriptorSize;

	UINT m_dwSwapChainFlags;
	UINT m_uiRenderTargetIndex;
	
	HANDLE m_hFenceEvent = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;

	DWORD m_dwCurContextIndex; // 현재 Drawcall을 받는 그룹의 Index이다.

	// window resizing
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	DWORD m_dwWidth;
	DWORD m_dwHeight;

	// Camera 
	FlyCamera* m_flyCamera;
	POINT m_LastMousePos;

	// Grid
	std::unique_ptr<class Grid_RenderMesh> m_pGridRenderMesh;
	XMMATRIX m_matGridWorld;

	// ScreenCapturer
	ScreenCapturer* m_pScreenStreamer;
	bool bTryPixelStreaming;
	bool bCheckUpdateTexture;

	// Font Manager
	float m_fDPI = 96.f;

	std::unordered_set<TEXTURE_HANDLE*> m_TextureHandles;

	// Default Assets
	TEXTURE_HANDLE* m_pDefaultWhiteTexture = nullptr;
	CONSTANT_BUFFER_MATERIAL m_DefaultMaterial;

	std::wstring m_strAssetRootPath;
	std::wstring m_strShaderRootPath;

public:
	D3D12Renderer();
	virtual ~D3D12Renderer();

	D3D12Device_raw INL_GetD3DDevice() const { return m_pD3DDevice.Get(); }
	D3D12ResourceManager* INL_GetResourceManager();
	ConstantBufferPool* INL_GetConstantBufferPool(E_CONSTANT_BUFFER_TYPE _type, ULONG _ulThreadIndex);
	DescriptorPool* INL_GetDescriptorPool(ULONG _ulThreadIndex);
	UINT INL_GetSrvDescriptorSize() const { return m_srvDescriptorSize; }
	SingleDescriptorAllocator* INL_GetSingleDescriptorAllocator();
	D3D12PSOCache* INL_GetD3D12PSOCache();
	void GetViewProjMatrix(XMMATRIX* _pOutMatView, XMMATRIX* _pOutMatProj);
	DWORD INL_GetScreenWidth() const { return m_dwWidth; }
	DWORD INL_GetScreenHeight() const { return m_dwHeight; }
	D3D12Resource_raw INL_GetFrameCBResource() const { return m_ppFrameUploadCBs[m_dwCurContextIndex].Get(); }
	float INL_GetDPI() const { return m_fDPI; }

	TEXTURE_HANDLE* INL_GetDefaultWhiteTexture() const { return m_pDefaultWhiteTexture; }
	CONSTANT_BUFFER_MATERIAL INL_GetDefaultMaterial() const { return m_DefaultMaterial; }

	XMFLOAT3 GetCameraWorldPos() const;
};

