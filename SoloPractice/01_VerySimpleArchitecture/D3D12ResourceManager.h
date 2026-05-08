// D3D12ResourceManager.h from "megayuchi"
#include "typedef.h"


#pragma once
// ID3D12Resource를 GPU 메모리에 올리는 클래스
class D3D12ResourceManager
{
public:
	bool Initialize(D3D12Device_ptr _pD3DDevice);

	HRESULT CreateVertexBuffer(
		UINT _sizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView,
		D3D12Resource_ptr* _ppOutBuffer, void* _pInitData);

	HRESULT CreateIndexBuffer(
		DWORD _dwIndexNum, D3D12_INDEX_BUFFER_VIEW* _pOutIndexBufferView,
		D3D12Resource_ptr* _ppOutBuffer, void* _pInitData, UINT _indexTypeSize = sizeof(uint16_t));

	HRESULT CreateTexture(D3D12Resource_ptr* _ppOutResource, UINT _width, UINT _height, DXGI_FORMAT _format, const BYTE* _pInitImage);
	HRESULT CreateTextureFromFile(D3D12Resource_ptr* _ppOutResource, D3D12_RESOURCE_DESC* _pOutDesc, const WCHAR* _wchFileName);
	HRESULT CreateTexturePair(D3D12Resource_ptr* _ppOutResource, D3D12Resource_ptr* _ppOutUploadBuffer, UINT _width, UINT _height, DXGI_FORMAT _format);

	void UpdateTextureForWrite(D3D12Resource_ptr _pDestTexResource, D3D12Resource_ptr _pSrcTexResource);
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
	
	D3D12Device_ptr m_pD3DDevice;
	// 클래스 내부에서 따로 queue와 allocator, list를 가진다.
	D3D12CommandQueue_ptr m_pCommandQueue;
	D3D12CommandAllocator_ptr m_pCommandAllocator;
	D3D12GraphicsCommandList_ptr m_pCommandList;

	HANDLE m_hFenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_ui64FenceValue;
	
public:
	D3D12ResourceManager();
	~D3D12ResourceManager();
};

