#pragma once

struct COMMAND_LIST
{
	D3D12CommandAllocator_ptr pDirectCommandAllocator;
	D3D12GraphicsCommandList_ptr pDirectCommandList;
	bool bClosed;
};

class CommandListPool
{
public:
	bool Initialize(D3D12Device_raw _pD3DDevice, D3D12_COMMAND_LIST_TYPE _type = D3D12_COMMAND_LIST_TYPE_DIRECT, ULONG _ulMaxCommandListCount = 2);
	D3D12GraphicsCommandList_raw GetCurrentCommandList();
	void CloseCurrentCommandList();
	void CloseAndExecuteCurrentCommandList(D3D12CommandQueue_raw _pCommandQueue);
	void ResetCommandListPool();

	ULONG GetTotalCommandListCount() const { return m_ulTotalCommandListCount; }
	ULONG GetAllocatedCommandListCount() const { return m_ulAllocatedCommandListCount; }
	ULONG GetAvailableCommandListCount() const { return m_ulAvailableCommandListCount; }

	D3D12Device_raw INL_GetD3DDevice() const { return m_pD3DDevice; }

	CommandListPool();
	virtual ~CommandListPool();
private:
	bool AddCommandList();
	COMMAND_LIST* AllocateCommandList();
	void CleanUpCommandLists();

private:
	D3D12Device_raw m_pD3DDevice;
	D3D12_COMMAND_LIST_TYPE m_CommandListType;

	ULONG m_ulAllocatedCommandListCount;
	ULONG m_ulAvailableCommandListCount;
	ULONG m_ulTotalCommandListCount;
	ULONG m_ulMaxCommandListCount;

	COMMAND_LIST* m_pCurrCommandList;
	std::vector<std::unique_ptr<COMMAND_LIST>> m_AvailableCommandLists;
	std::vector<std::unique_ptr<COMMAND_LIST>> m_AllocatedCommandLists;
};

