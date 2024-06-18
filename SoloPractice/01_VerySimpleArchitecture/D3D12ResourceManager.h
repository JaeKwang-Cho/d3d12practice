// D3D12ResourceManager.h from "megayuchi"


#pragma once
// ID3D12Resource를 GPU 메모리에 올리는 클래스
class D3D12ResourceManager
{
public:
	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice);
	HRESULT CreateVertexBuffer(
		UINT _sizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView,
		Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData);
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
	// 클래스 내부에서 따로 queue와 allocator, list를 가진다.
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

