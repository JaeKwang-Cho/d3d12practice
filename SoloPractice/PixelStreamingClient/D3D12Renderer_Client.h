#pragma once
class D3D12Renderer_Client
{
public:
	bool Initialize(HWND _hWnd);

	void BeginRender();
	void DrawStreamPixels(UINT8* _pPixels, UINT64 _ui64TotalBytes);
	void EndRender();
	void Present();

protected:
private:

public:
protected:
private:
	HWND m_hWnd;

	D3D_FEATURE_LEVEL m_FeatureLevel;
	DXGI_ADAPTER_DESC3 m_AdaptorDesc;
	Microsoft::WRL::ComPtr<ID3D12Device> m_pD3DDevice;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_pRenderTargets[2];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;

	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;

	UINT m_uiRenderTargetIndex;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pDefaultTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadTexture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSRVHeap;


	HANDLE m_hFenceEvent = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_ui64FenceValue;

	UINT m_dwSwapChainFlags;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	DWORD m_dwWidth;
	DWORD m_dwHeight;
};

