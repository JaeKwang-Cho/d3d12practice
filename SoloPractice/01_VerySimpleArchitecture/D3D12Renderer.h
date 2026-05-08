// D3D12Renderer.h from "megayuchi"
// 괜히 최신 d3d12 사용해보기
#pragma once

#include <unordered_set>

class D3D12ResourceManager;
class ConstantBufferPool;
class DescriptorPool;
class SingleDescriptorAllocator;
class ConstantBufferManager;
class D3D12PSOCache;
class FlyCamera;
class ScreenCapturer;
class FontManager;

class D3D12Renderer
{
public:
	static const UINT MAX_DRAW_COUNT_PER_FRAME = 256; // 한 프레임당 하나의 모델에 대해서 최대 그려질 횟수를 지정한다.
	static const UINT MAX_DESCRIPRTOR_COUNT = 4096; // Shader Resource View로서 Bind될 친구들의 최대 개수를 지정한다.
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV);

	void Update(const GameTimer& _gameTimer);
	void TryPixelStreaming() { bTryPixelStreaming = true; };

	void BeginRender();
	void CopyRenderTarget();
	void EndRender();
	void Present();

	bool UpdateWindowSize(DWORD _dwWidth, DWORD _dwHeight);

	// Render Mesh
	void DeleteRenderMesh(void* _pMeshObjectHandle, E_RENDER_MESH_TYPE _eRenderMeshType);
	void DrawRenderMesh(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld, E_RENDER_MESH_TYPE _eRenderMeshType);
	void DrawOutlineMesh(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld);
	
	// mesh
#if 0
	void* CreateBasicMeshObject();
	void DeleteBasicMeshObject(void* _pMeshObjectHandle);
	void RenderMeshObject(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld);

	bool BeginCreateMesh(void* _pMeshObjHandle, const ColorVertex* _pVertexList, DWORD _dwVertexCount, DWORD _dwTriGroupCount);
	bool InsertTriGroup(void* _pMeshObjHandle, const uint16_t* _pIndexList, DWORD _dwTriCount, const WCHAR* _wchTexFileName);
	void EndCreateMesh(void* _pMeshObjHandle);
#endif
	// sprite
	void* CreateSpriteObject();
	void* CreateSpriteObject(const WCHAR* _wchTexFileName, int _posX, int _posY, int _width, int _height);
	void DeleteSpriteObject(void* _pSpriteObjHandle);
	void RenderSpriteWithTex(void* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, const RECT* _pRect, float _z, void* _pTexHandle);
	void RenderSprite(void* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, float _z);
	void UpdateTextureWithImage(void* _pTextHandle, const BYTE* _pSrcBytes, UINT _SrcWidth, UINT _SrcHeight);

	// texture
	void* CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b);
	void* CreateTextureFromFile(const WCHAR* _wchFileName);
	void* CreateDynamicTexture(UINT _TexWidth, UINT _TexHeight);
	void DeleteTexture(void* _pTexHandle);

	// Font
	std::unique_ptr<FONT_HANDLE> CreateFontObject(const WCHAR* _wchFontFamilyName, float _fFontSize);
	bool WriteTextToBitmap(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, void* _pFontHandle, const WCHAR* _wchString, DWORD _dwLen);

	// PSO
	D3D12PipelineState_raw GetPSO(std::string _strPSOName);
	bool CachePSO(std::string _strPSOName, D3D12PipelineState_raw _pPSODesc);

	// Camera

	// Input
	void OnRButtonDown(WPARAM _btnState, int _x, int _y);
	void OnRButtonUp(WPARAM _btnState, int _x, int _y);
	void OnMouseMove(WPARAM _btnState, int _x, int _y);
	void OnKeyboardInput(const GameTimer& _gameTimer);

	void FlushMultiRendering();

protected:
private:
	void CreateCommandList();

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

	TEXTURE_HANDLE* AllocTextureHandle();
	void ReleaseAllTextureHandles();

public:

protected:
private:
	HWND m_hWnd;
	D3D12Device_ptr m_pD3DDevice; // 나중에 다 typedef로 바꾸기
	D3D12CommandQueue_ptr m_pCommandQueue;
	
	// 중첩 렌더링을 위해 Command Allocator와 Command List를 여러개 가진다.
	// 이러면 Fence가 좀더 여유로워 지고 GPU의 부하를 늘려줘서 프레임이 빨라진다.
	D3D12CommandAllocator_ptr m_ppCommandAllocator[MAX_PENDING_FRAME_COUNT];
	D3D12GraphicsCommandList_ptr m_ppCommandList[MAX_PENDING_FRAME_COUNT];
	// Frame 별 한번씩 넘어가는 CBV이다.
	D3D12Resource_ptr m_ppFrameUploadCBs[MAX_PENDING_FRAME_COUNT];
	void* m_ppFrameSystemMemAddrs[MAX_PENDING_FRAME_COUNT];

	// Resource를 GPU에 올려주는 친구
	std::unique_ptr<D3D12ResourceManager> m_pResourceManager;
	// CBV pool이랑 DescriptorPool 도 CommandList 마다 하나씩 만든다.
	// 얘도 Render Pipeline에 bind되어서 쓰이는 애들이다. CommandList만 분리해서는 절대 안된다.
	std::unique_ptr<ConstantBufferManager> m_ppConstantBufferManager[MAX_PENDING_FRAME_COUNT]; // 이제 pool에서 바로 빼오는 것이 아니라 manager를 통해서 가져온다.
	std::unique_ptr<DescriptorPool> m_ppDescriptorPool[MAX_PENDING_FRAME_COUNT];
	// Descriptor(View)를 모아서 관리해주는 친구
	std::unique_ptr<SingleDescriptorAllocator>	m_pSingleDescriptorAllocator;
	// PSO를 캐싱해주는 친구
	std::unique_ptr<D3D12PSOCache> m_pD3D12PSOCache;
	// Font Manager
	std::unique_ptr<FontManager> m_pFontManager;

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

	// ScreenCapturer
	ScreenCapturer* m_pScreenStreamer;
	bool bTryPixelStreaming;
	bool bCheckUpdateTexture;

	// Font Manager
	float m_fDPI = 96.f;

	std::unordered_set<TEXTURE_HANDLE*> m_TextureHandles;

public:
	D3D12Renderer();
	~D3D12Renderer();

	D3D12Device_raw INL_GetD3DDevice() { return m_pD3DDevice.Get(); }
	D3D12ResourceManager* INL_GetResourceManager();
	ConstantBufferPool* INL_GetConstantBufferPool(E_CONSTANT_BUFFER_TYPE _type);
	DescriptorPool* INL_DescriptorPool();
	UINT INL_GetSrvDescriptorSize() { return m_srvDescriptorSize; }
	SingleDescriptorAllocator* INL_GetSingleDescriptorAllocator();
	D3D12PSOCache* INL_GetD3D12PSOCache();
	void GetViewProjMatrix(XMMATRIX* _pOutMatView, XMMATRIX* _pOutMatProj);
	DWORD INL_GetScreenWidth() const { return m_dwWidth; }
	DWORD INL_GetScreenHeight() const { return m_dwHeight; }
	D3D12Resource_raw INL_GetFrameCBResource() { return m_ppFrameUploadCBs[m_dwCurContextIndex].Get(); }
	float INL_GetDPI() const { return m_fDPI; }


	XMFLOAT3 GetCameraWorldPos() const;
};

