#pragma once
#include <array>

struct SHADER_HANDLE;
class ShaderManager;
class RayTracingManager;
class ConstantBufferManager;
class SimpleConstantBufferPool;
class SingleDescriptorAllocator;
class D3D12ResourceManager;
class FontManager;
class TextureManager;
class DescriptorPool;

class D3D12Renderer
{
	static const UINT MAX_DRAW_COUNT_PER_FRAME = 1024;
	static const UINT MAX_DESCRIPTOR_COUNT = 4096;

public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader, const WCHAR* _wchSahderPath, ULONG _ulMaxBlasCount);
	void BeginRender();
	void EndRender();

	void Present();
	bool UpdateWindowSize(ULONG _width, ULONG _Height);

	void SetCameraPos(const float _x, const float _y, const float _z);
	void MoveCamera(const float _x, const float _y, const float _z);
	void GetCameraPos(float& _outX, float& _outY, float& _outZ);
	void ApplyCameraRot(const float _yaw, const float _pitch, const float _roll);

	// DXR On-Off
	void EnableDXR(bool _bEnable);
	bool IsEnabledDXR();

public:
	void* CreateBasicMeshObject();
	void DeleteBasicMeshObject(void* _pMeshObjHandle);

	void* CreateBLAS(void* _pMeshObjHandle);
	void DeleteBLAS(void* _pMeshObjHandle, void* _pBlasHandle);
	void UpdateBLASTransform(void* _pBlasHandle, const XMMATRIX* _pMatWorld);	// Transform RigidBody(MATRIX in TLAS)

	void* CreateSpriteObject();
	void* CreateSpriteObject(const WCHAR* _wchTexFileName, int _PosX, int _PosY, int _Width, int _Height);
	void DeleteSpriteObject(void* _pSpriteObjHandle);

	BOOL BeginCreateMesh(void* _pMeshObjHandle, const BasicVertex* _pVertexList, ULONG _ulVertexCount, ULONG _ulTriGroupCount);
	BOOL InsertTriGroup(void* _pMeshObjHandle, const USHORT* _pIndexList, ULONG _ulTriCount, const WCHAR* _wchTexFileName);
	void EndCreateMesh(void* _pMeshObjHandle);

	void* CreateTiledTexture(UINT _TexWidth, UINT _TexHeight, ULONG _r, ULONG _g, ULONG _b);
	void* CreateDynamicTexture(UINT _TexWidth, UINT _TexHeight);
	void* CreateTextureFromFile(const WCHAR* _wchFileName);
	void* CreateImmutableTexture(UINT _TexWidth, UINT _TexHeight, DXGI_FORMAT _format, const BYTE* _pInitImage);
	void DeleteTexture(void* _pTexHandle);

	void* CreateFontObject(const WCHAR* _wchFontFamilyName, float _fFontSize);
	void DeleteFontObject(void* _pFontHandle);
	bool WriteTextToBitmap(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, void* _pFontObjHandle, const WCHAR* _wchString, ULONG _ulLen);

	void RenderMeshObject(void* _pMeshObjHandle, const XMMATRIX* pMatWorld);
	void RenderSpriteWithTex(void* _pSprObjHandle, int _iPosX, int _iPosY, float _fScaleX, float _fScaleY, const RECT* _pRect, float _Z, void* _pTexHandle);
	void RenderSprite(void* _pSprObjHandle, int _iPosX, int _iPosY, float _fScaleX, float _fScaleY, float _Z);
	void UpdateTextureWithImage(void* _pTexHandle, const BYTE* _pSrcBits, UINT _SrcWidth, UINT _SrcHeight);


private:
	void CreateCommandList();
	bool CreateDescriptorHeapForRTV();
	bool CreateDescriptorHeapForDSV();

	bool CreateDepthStencilBuffer(UINT _Width, UINT _Height);
	void CleanupDepthStencilBuffer();

	void CreateFence();
	UINT64 DoFence();
	void WaitForFenceValue(UINT64 _ExpectedFenceValue);
	void CleanUpFence();

	void CleanupRenderer();

	void InitCamera();
	void UpdateCamera();

public:

private:
	HWND m_hWnd = nullptr;

	// Render가 소유한다.
	D3D12Device_ptr m_pD3DDevice = nullptr;
	D3D12CommandQueue_ptr m_pCommandQueue = nullptr;
	
	D3D12CommandAllocator_ptr m_ppCommandAllocator[MAX_PENDING_FRAME_COUNT] = {};
	D3D12GraphicsCommandList_ptr m_ppCommandList[MAX_PENDING_FRAME_COUNT] = {};
	std::array<std::unique_ptr<DescriptorPool>, MAX_PENDING_FRAME_COUNT> m_ppDescriptorPool = {};
	std::array<std::unique_ptr<ConstantBufferManager>, MAX_PENDING_FRAME_COUNT> m_ppConstantBufferManager = {};
	UINT64 m_pui64FenceValue[MAX_PENDING_FRAME_COUNT] = {};
	UINT64 m_ui64FenceValue = 0;

	// Manager도 Render가 소유한다.
	std::unique_ptr<ShaderManager> m_pShaderManager = nullptr;
	std::unique_ptr<RayTracingManager> m_pRayTracingManager = nullptr;
	std::unique_ptr<D3D12ResourceManager> m_pResourceManager = nullptr;
	std::unique_ptr<FontManager> m_pFontManager = nullptr;
	std::unique_ptr<TextureManager> m_pTextureManager = nullptr;

	std::unique_ptr< SingleDescriptorAllocator> m_pSingleDescriptorAllocator = nullptr;

	D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC3 m_adapterDesc = {};

	// SwapChain도 Render가 관리한다.
	DXGISwapChain_ptr m_pSwapChain = nullptr;
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	ULONG m_ulWidth = 0;
	ULONG m_ulHeight = 0;
	float m_fDPI = 96.f;

	// 렌더 타겟과 깊이버퍼도 Render가 관리한다.
	D3D12Resource_ptr m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	D3D12Resource_ptr m_pDepthStencilBuffer = nullptr;

	D3D12DescriptorHeap_ptr m_pRTVHeap = nullptr;
	D3D12DescriptorHeap_ptr m_pDSVHeap = nullptr;
	D3D12DescriptorHeap_ptr m_pSRVHeap = nullptr;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;
	UINT m_srvDescriptorSize = 0;

	UINT m_uiSwapChainFlags = 0;
	UINT m_uiRenderTargetIndex = 0;

	HANDLE m_hFenceEvent = nullptr;
	D3D12Fence_ptr m_pFence = nullptr;

	ULONG m_ulCurContextIndex = 0;

	// For Camera
	XMMATRIX m_matView = {};
	XMMATRIX m_matViewInv = {};
	XMMATRIX m_matProj = {};

	XMVECTOR m_vCamPos = {};
	XMVECTOR m_vCamDir = {};
	XMVECTOR m_vCamUp = {};
	XMVECTOR m_vCamRight = {};

	float m_fCamYaw = 0.f;
	float m_fCamPitch = 0.f;
	float m_fCamRoll = 0.f;
	
	bool m_bDXREnabled = true;

public:
	D3D12Device_raw INL_GetD3DDevice() const { return m_pD3DDevice.Get(); }
	ShaderManager* INL_GetShaderManager() const { return m_pShaderManager.get(); }
	RayTracingManager* INL_GetRayTracingManager() const { return m_pRayTracingManager.get(); }
	SimpleConstantBufferPool* INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE _cbType);
	D3D12ResourceManager* INL_GetResourceManager() const { return m_pResourceManager.get(); }
	DescriptorPool* INL_GetDescriptorPool() { return m_ppDescriptorPool[m_ulCurContextIndex].get(); }

	UINT INL_GetSrvDescriptorSize() { return m_srvDescriptorSize; }
	SingleDescriptorAllocator* INL_GetSingleDescriptorAllocator() { return m_pSingleDescriptorAllocator.get(); }


	void FillProjDecompConstant(DECOMP_PROJ* _pOutConstBuffer);
	void FillRayTraceConstant(CONSTANT_BUFFER_RAY_TRACING* _pOutBuffer);

	ULONG INL_GetWidth() const { return m_ulWidth; }
	ULONG INL_GetHeight() const { return m_ulHeight; }
	void INL_GetViewProjMatrix(XMMATRIX& _outView, XMMATRIX& _outProj);

	ULONG INL_GetScreenWidth() const { return m_ulWidth; }
	ULONG INL_GetScreenHeight() const { return m_ulHeight; }
	float INL_GetDPI() const { return m_fDPI; }

	D3D12Renderer();
	virtual ~D3D12Renderer();
};

