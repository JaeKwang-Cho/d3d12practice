#pragma once

struct SHADER_HANDLE;
class ShaderManager;
class RayTracingManager;

class D3D12Renderer
{
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader, const WCHAR* _wchSahderPath);
	void BeginRender();
	void EndRender();

	void Present();
	bool UpdateWindowSize(ULONG _width, ULONG _Height);

private:
	void CreateCommandList();
	void CleanupCommandList();

	bool CreateDescriptorHeapForRTV();
	void CleanupDescriptorHeapForRTV();
	bool CreateDescriptorHeapForDSV();
	void CleanupDescriptorHeapForDSV();

	bool CreateDepthStencilBuffer(UINT _Width, UINT _Height);
	void CleanupDepthStencilBuffer();

	void CreateFence();
	UINT64 DoFence();
	void WaitForFenceValue();
	void CleanUpFence();

	void Cleanup();

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

	D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_ADAPTER_DESC m_adapterDesc = {};

	// SwapChain도 Render가 관리한다.
	DXGISwapChain_ptr m_pSwapChain = nullptr;
	D3D12_VIEWPORT m_viewport = {};
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

public:
	D3D12Device_raw INL_GetD3DDevice() const { return m_pD3DDevice.Get(); }
	ShaderManager* INL_GetShaderManager() const { return m_pShaderManager.get(); }
	RayTracingManager* INL_GetRayTracingManager() const { return m_pRayTracingManager.get(); }

	ULONG INL_GetWidth() const { return m_ulWidth; }
	ULONG INL_GetHeight() const { return m_ulHeight; }

	D3D12Renderer();
	virtual ~D3D12Renderer();
};

