// D3D12Renderer.h from "megayuchi"
// ���� �ֽ� d3d12 ����غ���
#pragma once

// ����ü�� ������ 3���� �ø���.
const UINT SWAP_CHAIN_FRAME_COUNT = 3;
const UINT MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;

class D3D12ResourceManager;
class ConstantBufferPool;
class DescriptorPool;
class SingleDescriptorAllocator;
class ConstantBufferManager;
class D3D12PSOCache;

class D3D12Renderer
{
public:
	static const UINT MAX_DRAW_COUNT_PER_FRAME = 256; // �� �����Ӵ� �ϳ��� �𵨿� ���ؼ� �ִ� �׷��� Ƚ���� �����Ѵ�.
	static const UINT MAX_DESCRIPRTOR_COUNT = 4096; // Shader Resource View�μ� Bind�� ģ������ �ִ� ������ �����Ѵ�.
public:
	bool Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV);
	void BeginRender();
	void EndRender();
	void Present();

	bool UpdateWindowSize(DWORD _dwWidth, DWORD _dwHeight);

	// 
	void* CreateRenderMesh();
	void* CreateRenderMesh(std::vector<MeshData>& _ppMeshData, const UINT _meshDataCount);
	void DeleteRenderMesh(void* _pMeshObjectHandle);
	void DrawRenderMesh(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld);
	
	// mesh
	void* CreateBasicMeshObject();
	void DeleteBasicMeshObject(void* _pMeshObjectHandle);
	void RenderMeshObject(void* _pMeshObjectHandle, const XMMATRIX* pMatWorld);

	bool BeginCreateMesh(void* _pMeshObjHandle, const BasicVertex* _pVertexList, DWORD _dwVertexCount, DWORD _dwTriGroupCount);
	bool InsertTriGroup(void* _pMeshObjHandle, const uint16_t* _pIndexList, DWORD _dwTriCount, const WCHAR* _wchTexFileName);
	void EndCreateMesh(void* _pMeshObjHandle);

	// sprite
	void* CreateSpriteObject();
	void* CreateSpriteObject(const WCHAR* _wchTexFileName, int _posX, int _posY, int _width, int _height);
	void DeleteSpriteObject(void* _pSpriteObjHandle);
	void RenderSpriteWithTex(void* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, const RECT* _pRect, float _z, void* _pTexHandle);
	void RenderSprite(void* _pSpriteObjHandle, int _posX, int _posY, float _scaleX, float _scaleY, float _z);

	// texture
	void* CreateTileTexture(UINT _texWidth, UINT _texHeight, BYTE _r, BYTE _g, BYTE _b);
	void* CreateTextureFromFile(const WCHAR* _wchFileName);
	void DeleteTexture(void* _pTexHandle);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO(std::string _strPSOName);
	bool CachePSO(std::string _strPSOName, Microsoft::WRL::ComPtr<ID3D12PipelineState> _pPSODesc);
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
	void WaitForFenceValue(UINT64 _expectedFenceValue);

	void CleanUpRenderer();

public:

protected:
private:
	HWND m_hWnd;
	Microsoft::WRL::ComPtr<ID3D12Device14> m_pD3DDevice; 
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	
	// ��ø �������� ���� Command Allocator�� Command List�� ������ ������.
	// �̷��� Fence�� ���� �����ο� ���� GPU�� ���ϸ� �÷��༭ �������� ��������.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_ppCommandAllocator[MAX_PENDING_FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> m_ppCommandList[MAX_PENDING_FRAME_COUNT];

	// Resource�� GPU�� �÷��ִ� ģ��
	D3D12ResourceManager* m_pResourceManager;
	// CBV pool�̶� DescriptorPool �� CommandList ���� �ϳ��� �����.
	// �굵 Render Pipeline�� bind�Ǿ ���̴� �ֵ��̴�. CommandList�� �и��ؼ��� ���� �ȵȴ�.
	ConstantBufferManager* m_ppConstantBufferManager[MAX_PENDING_FRAME_COUNT]; // ���� pool���� �ٷ� ������ ���� �ƴ϶� manager�� ���ؼ� �����´�.
	DescriptorPool* m_ppDescriptorPool[MAX_PENDING_FRAME_COUNT];
	// Descriptor(View)�� ��Ƽ� �������ִ� ģ��
	SingleDescriptorAllocator* m_pSingleDescriptorAllocator;
	// PSO�� ĳ�����ִ� ģ��
	D3D12PSOCache* m_pD3D12PSOCache;

	UINT64 m_ui64FenceValue;
	// CommandList ���� ��ٸ��⸦ �ٶ�� Fence Value�� �����Ѵ�.
	UINT64 m_pui64LastFenceValue[MAX_PENDING_FRAME_COUNT];

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

	DWORD m_dwCurContextIndex; // ���� Drawcall�� �޴� �׷��� Index�̴�.

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
	ConstantBufferPool* INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE _type);
	DescriptorPool* INL_DescriptorPool() { return m_ppDescriptorPool[m_dwCurContextIndex]; }
	UINT INL_GetSrvDescriptorSize() { return m_srvDescriptorSize; }
	SingleDescriptorAllocator* INL_GetSingleDescriptorAllocator() { return m_pSingleDescriptorAllocator; }
	D3D12PSOCache* INL_GetD3D12PSOCache() { return m_pD3D12PSOCache; }
	void GetViewProjMatrix(XMMATRIX* _pOutMatView, XMMATRIX* _pOutMatProj) {
		*_pOutMatView = m_matView;
		*_pOutMatProj = m_matProj;
		//*_pOutMatView = XMMatrixTranspose(m_matView);
		//*_pOutMatProj = XMMatrixTranspose(m_matProj);
	}
	DWORD INL_GetScreenWidth() const { return m_dwWidth; }
	DWORD INL_GetScreenHeight() const { return m_dwHeight; }

};

