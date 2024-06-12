// D3D12Renderer.h from "megayuchi"
// 괜히 최신 d3d12 사용해보기
#pragma once

const UINT SWAP_CHAIN_FRAME_COUNT = 2;

class D3D12Renderer
{
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV);
	void BeginRender();
	void EndRender();
	void Present();
protected:
private:
	void CreateCommandList();
	bool CreateDescriptorHeap();
	void CreateFence();
	void CleanupFence();

	UINT64 GetFence();
	void WaitForFenceValue();

	void CleanUpRenderer();

public:

protected:
private:
	HWND m_hWnd;
	Microsoft::WRL::ComPtr<ID3D12Device14> m_pD3DDevice; // 잘 모르지만 제일 최신거 사용해보기
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> m_pCommandList;// 잘 모르지만 제일 최신거 사용해보기22

	UINT64 m_ui64enceValue;

	D3D_FEATURE_LEVEL m_FeatureLevel;
	DXGI_ADAPTER_DESC3 m_AdaptorDesc;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSRVHeap;

	UINT m_rtvDescriptorSize;
	UINT m_dwSwapChainFlags;
	UINT m_uiRenderTargetIndex;
	
	HANDLE m_hFenceEvent = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;

	DWORD m_dwCurContextIndex;

public:
	D3D12Renderer();
	~D3D12Renderer();
};

