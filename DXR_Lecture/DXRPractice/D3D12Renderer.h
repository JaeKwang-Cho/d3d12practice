#pragma once

struct SHADER_HANDLE;
class ShaderManager;
class RayTracingManager;
class ConstantBufferManager;
class SimpleConstantBufferPool;
class D3D12ResourceManager;

class D3D12Renderer
{
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader, const WCHAR* _wchSahderPath);
	void BeginRender();
	void EndRender();

	void Present();
	bool UpdateWindowSize(ULONG _width, ULONG _Height);

	void SetCameraPos(const float _x, const float _y, const float _z);
	void MoveCamera(const float _x, const float _y, const float _z);
	void GetCameraPos(float& _outX, float& _outY, float& _outZ);
	void ApplyCameraRot(const float _yaw, const float _pitch, const float _roll);

private:
	void CreateCommandList();
	bool CreateDescriptorHeapForRTV();
	bool CreateDescriptorHeapForDSV();

	bool CreateDepthStencilBuffer(UINT _Width, UINT _Height);
	void CleanupDepthStencilBuffer();

	void CreateFence();
	UINT64 DoFence();
	void WaitForFenceValue();
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
	
	D3D12CommandAllocator_ptr m_pCommandAllocator = nullptr;
	D3D12GraphicsCommandList_ptr m_pCommandList = nullptr;
	UINT64 m_ui64FenceValue = 0;

	// Manager도 Render가 소유한다.
	std::unique_ptr<ShaderManager> m_pShaderManager = nullptr;
	std::unique_ptr<RayTracingManager> m_pRayTracingManager = nullptr;
	std::unique_ptr<ConstantBufferManager> m_pConstantBufferManager = nullptr;
	std::unique_ptr<D3D12ResourceManager> m_pResourceManager = nullptr;

	D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC3 m_adapterDesc = {};

	// SwapChain도 Render가 관리한다.
	DXGISwapChain_ptr m_pSwapChain = nullptr;
	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	ULONG m_ulWidth = 0;
	ULONG m_ulHeight = 0;

	// 렌더 타겟과 깊이버퍼도 Render가 관리한다.
	D3D12Resource_ptr m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	D3D12Resource_ptr m_pDepthStencilBuffer = nullptr;

	D3D12DescriptorHeap_ptr m_pRTVHeap = nullptr;
	D3D12DescriptorHeap_ptr m_pDSVHeap = nullptr;

	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;

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

public:
	D3D12Device_raw INL_GetD3DDevice() const { return m_pD3DDevice.Get(); }
	ShaderManager* INL_GetShaderManager() const { return m_pShaderManager.get(); }
	RayTracingManager* INL_GetRayTracingManager() const { return m_pRayTracingManager.get(); }
	SimpleConstantBufferPool* INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE _cbType);
	D3D12ResourceManager* INL_GetResourceManager() const { return m_pResourceManager.get(); }

	void FillProjDecompConstant(DECOMP_PROJ* _pOutConstBuffer);
	void FillRayTraceConstant(CONSTANT_BUFFER_RAY_TRACING* _pOutBuffer);

	ULONG INL_GetWidth() const { return m_ulWidth; }
	ULONG INL_GetHeight() const { return m_ulHeight; }
	void INL_GetViewProjMatrix(XMMATRIX& _outView, XMMATRIX& _outProj);

	D3D12Renderer();
	virtual ~D3D12Renderer();
};

