// D3D12ResourceManager.h from "megayuchi"


#pragma once
// ID3D12Resource�� GPU �޸𸮿� �ø��� Ŭ����
class D3D12ResourceManager
{
public:
	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice);

	HRESULT CreateVertexBuffer(
		UINT _sizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView,
		Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData);

	HRESULT CreateIndexBuffer(
		DWORD _dwIndexNum, D3D12_INDEX_BUFFER_VIEW* _pOutIndexBufferView,
		Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData, UINT _indexTypeSize = sizeof(uint16_t));

	HRESULT CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutResource, UINT _width, UINT _height, DXGI_FORMAT _format, const BYTE* _pInitImage);
	HRESULT CreateTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutResource, D3D12_RESOURCE_DESC* _pOutDesc, const WCHAR* _wchFileName);

	void UpdateTextureForWrite(Microsoft::WRL::ComPtr<ID3D12Resource> _pDestTexResource, Microsoft::WRL::ComPtr<ID3D12Resource> _pSrcTexResource);
protected:
private:

	void CreateFence();
	void CleanUpFence();
	void CreateCommandList();

	UINT64 DoFence();
	void WaitForFenceValue();
	void CleanUpManager();
	
public:
protected:
private:
	
	Microsoft::WRL::ComPtr<ID3D12Device14> m_pD3DDevice;
	// Ŭ���� ���ο��� ���� queue�� allocator, list�� ������.
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> m_pCommandList;

	HANDLE m_hFenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_ui64FenceValue;
	
public:
	D3D12ResourceManager();
	~D3D12ResourceManager();
};

