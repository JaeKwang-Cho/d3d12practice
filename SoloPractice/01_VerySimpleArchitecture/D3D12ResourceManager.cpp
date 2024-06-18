// D3D12ResourceManager.h from "megayuchi"


#include "pch.h"
#include "D3D12ResourceManager.h"

bool D3D12ResourceManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice)
{
	bool bResult = false;
	m_pD3DDevice = _pD3DDevice;

	// Queue를 새로이 생성한다.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
	if (FAILED(hr)) {
		__debugbreak();
		goto RETURN;
	}
	// CommandList도 클래스 용으로 하나 더 만든다.
	CreateCommandList();
	// GPU에 메모리를 올리는 거라서 필요하다.
	CreateFence();

RETURN:
	return bResult;
}

HRESULT D3D12ResourceManager::CreateVertexBuffer(UINT _sizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData)
{
	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	// 업로드 버퍼를 이용해서, 기본 버퍼로 데이터를 전달한다.
	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT vertexBufferSize = _sizePerVertex * _dwVertexNum;

	D3D12_HEAP_PROPERTIES heapProps_Default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES heapProps_Upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC vbDesc_BuffSize = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	// 이렇게 Default 버퍼와
	hr = m_pD3DDevice->CreateCommittedResource(
		&heapProps_Default,
		D3D12_HEAP_FLAG_NONE,
		&vbDesc_BuffSize,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
			IID_PPV_ARGS(pVertexBuffer.GetAddressOf())
			);

			if (FAILED(hr)) {
				__debugbreak();
				goto RETURN;
			}

			if (_pInitData) {
				if (FAILED(m_pCommandAllocator->Reset())) {
					__debugbreak();
				}
				if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr))) {
					__debugbreak();
				}
				// 업로드 버퍼를 하나씩 만들어 준다.
				hr = m_pD3DDevice->CreateCommittedResource(
					&heapProps_Upload,
					D3D12_HEAP_FLAG_NONE,
					&vbDesc_BuffSize,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
				);

				if (FAILED(hr)) {
					__debugbreak();
					goto RETURN;
				}

				// 일단 삼각형 데이터를 uploadBuffer에 복사해준다.
				UINT8* pVertexDataBegin = nullptr;
				CD3DX12_RANGE writeRange(0, 0); // 일단 깡통 Range를 만들어 준다. CPU에서 값을 읽지 않을 것이다.

				hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pVertexDataBegin));
				if (FAILED(hr)) {
					__debugbreak();
					goto RETURN;
				}
				memcpy(pVertexDataBegin, _pInitData, vertexBufferSize);
				pUploadBuffer->Unmap(0, nullptr);

				// 이제 Upload Buffer에 있는 값을 Default 버퍼로 복사한다.
				D3D12_RESOURCE_BARRIER vbRB_COMM_DEST = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				D3D12_RESOURCE_BARRIER vbRB_DEST_BUFF = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

				m_pCommandList->ResourceBarrier(1, &vbRB_COMM_DEST);
				m_pCommandList->CopyBufferRegion(pVertexBuffer.Get(), 0, pUploadBuffer.Get(), 0, vertexBufferSize);
				m_pCommandList->ResourceBarrier(1, &vbRB_DEST_BUFF);

				m_pCommandList->Close();

				ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
				m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

				// GPU에 생성하는 것이기 때문에 fence로 기다린다.
				DoFence();
				WaitForFenceValue();
			}
			// Vertex Buffer View 값을 채워준다.
			vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
			vertexBufferView.StrideInBytes = _sizePerVertex;
			vertexBufferView.SizeInBytes = vertexBufferSize;

			// 리턴을 해준다.
			*_pOutVertexBufferView = vertexBufferView;
			*_ppOutBuffer = pVertexBuffer;

		RETURN:
			// fence를 쳐서 기다렸기 때문에 upload 버퍼는 없애줘도 된다.
			return hr;
}

void D3D12ResourceManager::CreateFence()
{
	// GPU 에 생성하는 것이라서 동기화를 시켜줘야 한다.
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}

	m_ui64FenceValue = 0;

	// 싱크는 윈도우 이벤트를 이용한다.
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void D3D12ResourceManager::CleanUpFence()
{
	if (m_hFenceEvent) {
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
}

void D3D12ResourceManager::CreateCommandList()
{
	// 일단 만들기는 하는데
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf())))) {
		__debugbreak();
	}
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())))){
		__debugbreak();
	}
	// 얘는 resource를 올리는데 사용하는 친구여서 일단 닫아 놓는다.
	m_pCommandList->Close();
}

UINT64 D3D12ResourceManager::DoFence()
{
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	return m_ui64FenceValue;
}

void D3D12ResourceManager::WaitForFenceValue()
{
	const UINT64 ExpectedFenceValue = m_ui64FenceValue;

	if (m_pFence->GetCompletedValue() < ExpectedFenceValue) {
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3D12ResourceManager::CleanUpManager()
{
	CleanUpFence();
}

D3D12ResourceManager::D3D12ResourceManager()
	:m_pD3DDevice(nullptr), m_pCommandQueue(nullptr), m_pCommandAllocator(nullptr),
	m_pCommandList(nullptr), m_hFenceEvent(nullptr), m_pFence(nullptr), m_ui64FenceValue(0)
{
}

D3D12ResourceManager::~D3D12ResourceManager()
{
	CleanUpManager();
}
