// D3D12Renderer.h from "megayuchi"
// 괜히 최신 d3d12 사용해보기
#pragma once

const UINT SWAP_CHAIN_FRAME_COUNT = 2;

class D3D12ResourceManager;
class ConstantBufferPool;
class DescriptorPool;
class SingleDescriptorAllocator;

class D3D12Renderer
{
public:
	static const UINT MAX_DRAW_COUNT_PER_FRAME = 256; // 한 프레임당 하나의 모델에 대해서 최대 그려질 횟수를 지정한다.
	static const UINT MAX_DESCRIPRTOR_COUNT = 4096; // Shader Resource View로서 Bind될 친구들의 최대 개수를 지정한다.
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV);
	void BeginRender();
	void EndRender();
	void Present();

	bool UpdateWindowSize(DWORD _dwWidth, DWORD _dwHeight);
	
	void* CreateBasicMeshObject_Return_New();
	void DeleteBasicMeshObject(void* _pMeshObjectHandle);
	void RenderMeshObject(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld, void* _pTexHandle);

	void* CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b);
	void* CreateTextureFromFile(const WCHAR* _wchFileName);
	void DeleteTexture(void* _pTexHandle);
protected:
private:
	void CreateCommandList();

	bool CreateDescriptorHeapForRTV();
	bool CreateDescriptorHeapForDSV();
	bool CreateDepthStencil(UINT _width, UINT _height);

	void InitCamera();

	void CreateFence();
	void CleanupFence();

	UINT64 DoFence();
	void WaitForFenceValue();

	void CleanUpRenderer();

public:

protected:
private:
	HWND m_hWnd;
	Microsoft::WRL::ComPtr<ID3D12Device14> m_pD3DDevice; 
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> m_pCommandList;

	// Resource를 GPU에 올려주는 친구
	D3D12ResourceManager* m_pResourceManager;
	// CBV pool
	ConstantBufferPool* m_pConstantBufferPool;
	// DescriptorPool
	DescriptorPool* m_pDescriptorPool;
	// Descriptor(View)를 모아서 관리해주는 친구
	SingleDescriptorAllocator* m_pSingleDescriptorAllocator;

	UINT64 m_ui64enceValue;

	D3D_FEATURE_LEVEL m_FeatureLevel;
	DXGI_ADAPTER_DESC3 m_AdaptorDesc;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_pDepthStencil;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSRVHeap;

	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;
	UINT m_dsvDescriptorSize;

	UINT m_dwSwapChainFlags;
	UINT m_uiRenderTargetIndex;
	
	HANDLE m_hFenceEvent = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;

	DWORD m_dwCurContextIndex; //?

	// window resizing
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	DWORD m_dwWidth;
	DWORD m_dwHeight;

	// Camera 
	XMMATRIX m_matView;
	XMMATRIX m_matProj;

public:
	D3D12Renderer();
	~D3D12Renderer();

	Microsoft::WRL::ComPtr<ID3D12Device14> INL_GetD3DDevice() { return m_pD3DDevice; }
	D3D12ResourceManager* INL_GetResourceManager() { return m_pResourceManager; }
	ConstantBufferPool* INL_GetConstantBufferPool() { return m_pConstantBufferPool; }
	DescriptorPool* INL_DescriptorPool() { return m_pDescriptorPool; }
	UINT INL_GetSrvDescriptorSize() { return m_srvDescriptorSize; }
	SingleDescriptorAllocator* INL_GetSingleDescriptorAllocator() { return m_pSingleDescriptorAllocator; }
	void GetViewProjMatrix(XMMATRIX* _pOutMatView, XMMATRIX* _pOutMatProj) {
		*_pOutMatView = m_matView;
		*_pOutMatProj = m_matProj;
		//*_pOutMatView = XMMatrixTranspose(m_matView);
		//*_pOutMatProj = XMMatrixTranspose(m_matProj);
	}
};

