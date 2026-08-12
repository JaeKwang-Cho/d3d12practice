#pragma once
class D3D12ResourceManager
{
public:
	bool Initialize(D3D12Device_ptr _pD3DDevice);

	HRESULT CreateVertexBuffer(
		UINT _sizePerVertex, ULONG _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView,
		D3D12Resource_raw* _ppOutBuffer, void* _pInitData);

	HRESULT CreateIndexBuffer(
		DWORD _dwIndexNum, D3D12_INDEX_BUFFER_VIEW* _pOutIndexBufferView,
		D3D12Resource_raw* _ppOutBuffer, void* _pInitData, UINT _indexTypeSize = sizeof(uint16_t));

	HRESULT CreateTexture(D3D12Resource_ptr* _ppOutResource, UINT _width, UINT _height, DXGI_FORMAT _format, const BYTE* _pInitImage);
	HRESULT CreateTextureFromFile(D3D12Resource_ptr* _ppOutResource, D3D12_RESOURCE_DESC* _pOutDesc, const WCHAR* _wchFileName);
	HRESULT CreateTexturePair(D3D12Resource_ptr* _ppOutResource, D3D12Resource_ptr* _ppOutUploadBuffer, UINT _width, UINT _height, DXGI_FORMAT _format);

	void UpdateTextureForWrite(D3D12Resource_ptr _pDestTexResource, D3D12Resource_ptr _pSrcTexResource);
protected:
private:
	void CreateFence_forResourceManager();
	void CleanUpFence_forResourceManager();
	void CreateCommandList_forResourceManager();

	UINT64 DoFence_forResourceManager();
	void WaitForFenceValue_forResourceManager();
	void CleanUp_forResourceManager();

public:
protected:
private:

	D3D12Device_ptr m_pD3DDevice = nullptr;
	// 클래스 내부에서 따로 queue와 allocator, list를 가진다.
	D3D12CommandQueue_ptr m_pCommandQueue = nullptr;
	D3D12CommandAllocator_ptr m_pCommandAllocator = nullptr;
	D3D12GraphicsCommandList_ptr m_pCommandList = nullptr;

	HANDLE m_hFenceEvent = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence = nullptr;
	UINT64 m_ui64FenceValue = 0;

public:
	D3D12ResourceManager();
	~D3D12ResourceManager();
};

