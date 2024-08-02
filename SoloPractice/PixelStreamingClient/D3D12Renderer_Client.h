#pragma once
#include "ReceiveStreaming.h"

class D3D12Renderer_Client
{
	static const UINT SWAP_CHAIN_FRAME_COUNT = 3;
public:
	bool Initialize(HWND _hWnd);

	void DrawStreamPixels();
protected:
private:
	bool CheckPixelReady();
	void BeginRender();
	void UploadStreamPixels();
	void SkipCurrentFrame();
	void EndRender();
	void Present();

	UINT64 DoCopyFence();
	void WaitForCopyFenceValue(UINT64 _expectedFenceValue);

	void IncreaseFenceValue_Wait();

	void CleanUpRenderer();
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
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;

	UINT m_rtvDescriptorSize;
	//UINT m_srvDescriptorSize;

	//Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	//Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

	//Microsoft::WRL::ComPtr<ID3D12Resource> m_pDefaultTexture[THREAD_NUMBER_BY_FRAME];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadTexture[SWAP_CHAIN_FRAME_COUNT];
	UINT8* m_ppMappedData[SWAP_CHAIN_FRAME_COUNT];

	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSRVHeap;
	UINT m_uiTextureIndexByThread;


	HANDLE m_hCopyFenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_ui64FenceValue;
	UINT64 m_pui64CopyFenceValue[SWAP_CHAIN_FRAME_COUNT];

	UINT m_dwSwapChainFlags;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	DWORD m_dwWidth;
	DWORD m_dwHeight;

	SRWLOCK m_srwLock;

	WinSock_Props* m_WinSock_Props;
	UINT64 m_TextureSize;
public:
	D3D12Renderer_Client();
	virtual~D3D12Renderer_Client();
};

