#include "pch.h"
#include "CommandListPool.h"

bool CommandListPool::Initialize(D3D12Device_raw _pD3DDevice, D3D12_COMMAND_LIST_TYPE _type, ULONG _ulMaxCommandListCount)
{
	if (_ulMaxCommandListCount < 2)
		__debugbreak();
	
	m_ulMaxCommandListCount = _ulMaxCommandListCount;
	m_CommandListType = _type;
	m_pD3DDevice = _pD3DDevice;

	return true;
}

D3D12GraphicsCommandList_raw CommandListPool::GetCurrentCommandList()
{
	if (!m_pCurrCommandList) {
		if (!(m_pCurrCommandList = AllocateCommandList())) {
#ifdef _DEBUG
			__debugbreak();
#endif
			return nullptr;
		}
	}
	return m_pCurrCommandList->pDirectCommandList.Get();
}

void CommandListPool::CloseCurrentCommandList()
{
	if (!m_pCurrCommandList) {
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}
	if (m_pCurrCommandList->bClosed) {
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	if(FAILED(m_pCurrCommandList->pDirectCommandList->Close()))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	m_pCurrCommandList->bClosed = true;
	m_pCurrCommandList = nullptr;
}

void CommandListPool::CloseAndExecuteCurrentCommandList(D3D12CommandQueue_raw _pCommandQueue)
{
	if (!m_pCurrCommandList) {
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}
	if (m_pCurrCommandList->bClosed) {
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	if(FAILED(m_pCurrCommandList->pDirectCommandList->Close()))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	m_pCurrCommandList->bClosed = true;

	ID3D12CommandList* ppCommandLists[] = { m_pCurrCommandList->pDirectCommandList.Get() };
	_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	m_pCurrCommandList = nullptr;
}

void CommandListPool::ResetCommandListPool()
{
	while (m_AllocatedCommandLists.size() > 0)
	{
		std::unique_ptr<COMMAND_LIST> pCommandListUnit = std::move(m_AllocatedCommandLists.back());
		m_AllocatedCommandLists.pop_back();
		m_ulAllocatedCommandListCount--;

		if (FAILED(pCommandListUnit->pDirectCommandAllocator->Reset()))
		{
#ifdef _DEBUG
			__debugbreak();
#endif
		}

		if(FAILED(pCommandListUnit->pDirectCommandList->Reset(pCommandListUnit->pDirectCommandAllocator.Get(), nullptr)))
		{
#ifdef _DEBUG
			__debugbreak();
#endif
		}

		pCommandListUnit->bClosed = false;

		m_AvailableCommandLists.push_back(std::move(pCommandListUnit));
		m_ulAvailableCommandListCount++;
	}
}

CommandListPool::CommandListPool():
	m_pD3DDevice(nullptr),
	m_CommandListType(D3D12_COMMAND_LIST_TYPE_DIRECT),
	m_ulAllocatedCommandListCount(0),
	m_ulAvailableCommandListCount(0),
	m_ulTotalCommandListCount(0),
	m_ulMaxCommandListCount(0),
	m_pCurrCommandList(nullptr)
{
}

bool CommandListPool::AddCommandList()
{
	bool bResult = false;

	std::unique_ptr<COMMAND_LIST> pCommandListUnit = std::make_unique<COMMAND_LIST>();
	D3D12CommandAllocator_ptr pCommandAllocator = nullptr;
	D3D12GraphicsCommandList_ptr pCommandList = nullptr;

	if (m_ulTotalCommandListCount >= m_ulMaxCommandListCount)
		return bResult;

	if(FAILED(m_pD3DDevice->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(pCommandAllocator.GetAddressOf()))))
	{
#ifdef _DEBUG
		__debugbreak();
#endif	
		return bResult;
	}
	
	if(FAILED(m_pD3DDevice->CreateCommandList(0, m_CommandListType, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(pCommandList.GetAddressOf()))))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return bResult;
	}
	pCommandListUnit->pDirectCommandAllocator = pCommandAllocator;
	pCommandListUnit->pDirectCommandList = pCommandList;
	m_ulTotalCommandListCount++;

	m_AvailableCommandLists.push_back(std::move(pCommandListUnit));
	m_ulAvailableCommandListCount++;

	bResult = true;
	return bResult;
}

COMMAND_LIST* CommandListPool::AllocateCommandList()
{
	COMMAND_LIST* pCmdListUnit = nullptr;

	if(m_ulTotalCommandListCount == 0 || m_ulAvailableCommandListCount == 0)
		if(!AddCommandList())
			return pCmdListUnit;
	
	std::unique_ptr<COMMAND_LIST> pCommandListUnit = std::move(m_AvailableCommandLists.back());
	m_AvailableCommandLists.pop_back();
	m_ulAvailableCommandListCount--;

	pCmdListUnit = pCommandListUnit.get();

	m_AllocatedCommandLists.push_back(std::move(pCommandListUnit));
	m_ulAllocatedCommandListCount++;

	return pCmdListUnit;
}

void CommandListPool::CleanUpCommandLists()
{
	ResetCommandListPool();

	while (m_AvailableCommandLists.size() > 0)
	{
		m_AvailableCommandLists.pop_back();
		m_ulAvailableCommandListCount--;
	}
}

CommandListPool::~CommandListPool()
{
	CleanUpCommandLists();
}