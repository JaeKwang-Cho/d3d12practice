// D3D12ResourceManager.cpp from "megayuchi"


#include "pch.h"
#include "D3D12ResourceManager.h"

bool D3D12ResourceManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice)
{
	bool bResult = false;
	m_pD3DDevice = _pD3DDevice;

	// Queue�� ������ �����Ѵ�.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
	if (FAILED(hr)) {
		__debugbreak();
		goto RETURN;
	}
	// CommandList�� Ŭ���� ������ �ϳ� �� �����.
	CreateCommandList();
	// GPU�� �޸𸮸� �ø��� �Ŷ� �ʿ��ϴ�.
	CreateFence();

RETURN:
	return bResult;
}

HRESULT D3D12ResourceManager::CreateVertexBuffer(UINT _sizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData)
{
	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	// ���ε� ���۸� �̿��ؼ�, �⺻ ���۷� �����͸� �����Ѵ�.
	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT vertexBufferSize = _sizePerVertex * _dwVertexNum;

	D3D12_HEAP_PROPERTIES heapProps_Default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES heapProps_Upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC vbDesc_BuffSize = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	// �̷��� Default ���ۿ�
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
				// ���ε� ���۸� �ϳ��� ����� �ش�.
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

				// �ϴ� �ﰢ�� �����͸� uploadBuffer�� �������ش�.
				UINT8* pVertexDataBegin = nullptr;
				CD3DX12_RANGE writeRange(0, 0); // �ϴ� ���� Range�� ����� �ش�. CPU���� ���� ���� ���� ���̴�.

				hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pVertexDataBegin));
				if (FAILED(hr)) {
					__debugbreak();
					goto RETURN;
				}
				memcpy(pVertexDataBegin, _pInitData, vertexBufferSize);
				pUploadBuffer->Unmap(0, nullptr);

				// ���� Upload Buffer�� �ִ� ���� Default ���۷� �����Ѵ�.
				D3D12_RESOURCE_BARRIER vbRB_COMM_DEST = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				D3D12_RESOURCE_BARRIER vbRB_DEST_BUFF = CD3DX12_RESOURCE_BARRIER::Transition(pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

				m_pCommandList->ResourceBarrier(1, &vbRB_COMM_DEST);
				m_pCommandList->CopyBufferRegion(pVertexBuffer.Get(), 0, pUploadBuffer.Get(), 0, vertexBufferSize);
				m_pCommandList->ResourceBarrier(1, &vbRB_DEST_BUFF);

				m_pCommandList->Close();

				ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
				m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

				// GPU�� �����ϴ� ���̱� ������ fence�� ��ٸ���.
				DoFence();
				WaitForFenceValue();
			}
			// Vertex Buffer View ���� ä���ش�.
			vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
			vertexBufferView.StrideInBytes = _sizePerVertex;
			vertexBufferView.SizeInBytes = vertexBufferSize;

			// ������ ���ش�.
			*_pOutVertexBufferView = vertexBufferView;
			*_ppOutBuffer = pVertexBuffer;

		RETURN:
			// fence�� �ļ� ��ٷȱ� ������ upload ���۴� �����൵ �ȴ�.
			return hr;
}

HRESULT D3D12ResourceManager::CreateIndexBuffer(DWORD _dwIndexNum, D3D12_INDEX_BUFFER_VIEW* _pOutIndexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData)
{
	HRESULT hr = S_OK;

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT indexBufferSize = sizeof(uint16_t) * _dwIndexNum;

	// Index�� upload heap buffer�� �̿��ؼ� default heap buffer�� �����͸� �ø���.
	D3D12_HEAP_PROPERTIES heapProps_Default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES heapProps_Upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC ibDesc_BuffSize = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

	hr = m_pD3DDevice->CreateCommittedResource(
		&heapProps_Default,
		D3D12_HEAP_FLAG_NONE,
		&ibDesc_BuffSize,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pIndexBuffer.GetAddressOf())
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

		hr = m_pD3DDevice->CreateCommittedResource(
			&heapProps_Upload,
			D3D12_HEAP_FLAG_NONE,
			&ibDesc_BuffSize,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
		);
		if (FAILED(hr)) {
			__debugbreak();
			goto RETURN;
		}

		// Index ������ upload buffer�� �ø���.
		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pIndexDataBegin));
		if (FAILED(hr)) {
			__debugbreak();
			goto RETURN;
		}
		memcpy(pIndexDataBegin, _pInitData, indexBufferSize);
		pUploadBuffer->Unmap(0, nullptr);

		// ���� Upload Buffer�� �ִ� ���� Default ���۷� �����Ѵ�.
		D3D12_RESOURCE_BARRIER ibRB_COMM_DEST = CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		D3D12_RESOURCE_BARRIER ibRB_DEST_INDEX = CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		m_pCommandList->ResourceBarrier(1, &ibRB_COMM_DEST);
		m_pCommandList->CopyBufferRegion(pIndexBuffer.Get(), 0, pUploadBuffer.Get(), 0, indexBufferSize);
		m_pCommandList->ResourceBarrier(1, &ibRB_DEST_INDEX);

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };

		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		DoFence();
		WaitForFenceValue();
	}

	// Index Buffer View�� ä���.
	indexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = indexBufferSize;

	*_pOutIndexBufferView = indexBufferView;
	*_ppOutBuffer = pIndexBuffer;

RETURN:
	return hr;
}

void D3D12ResourceManager::CreateFence()
{
	// GPU �� �����ϴ� ���̶� ����ȭ�� ������� �Ѵ�.
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}

	m_ui64FenceValue = 0;

	// ��ũ�� ������ �̺�Ʈ�� �̿��Ѵ�.
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
	// �ϴ� ������ �ϴµ�
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf())))) {
		__debugbreak();
	}
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())))){
		__debugbreak();
	}
	// ��� resource�� �ø��µ� ����ϴ� ģ������ �ϴ� �ݾ� ���´�.
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
