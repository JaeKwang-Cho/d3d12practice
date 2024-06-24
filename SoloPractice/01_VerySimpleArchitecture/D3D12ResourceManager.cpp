// D3D12ResourceManager.cpp from "megayuchi"


#include "pch.h"
#include "D3D12ResourceManager.h"
#include "DirectXTexHeader.h"
#include "DDSTextureLoader12.h"

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

HRESULT D3D12ResourceManager::CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutResource, UINT _width, UINT _height, DXGI_FORMAT _format, const BYTE* _pInitImage)
{
	HRESULT hr = S_OK;

	// �ؽ��ĵ� ���������� upload -> default�� �̿��ؼ� GPU�� �ø���.
	Microsoft::WRL::ComPtr<ID3D12Resource> pTexResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = _format;
	textureDesc.Width = _width;
	textureDesc.Height = _height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_HEAP_PROPERTIES heapProps_Default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES heapProps_Upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	hr = m_pD3DDevice->CreateCommittedResource(
		&heapProps_Default,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, // ���̴����� ����ϴ� ���ҽ��� �׳� �̰ɷ� ��ġ�� ����
		nullptr,
		IID_PPV_ARGS(pTexResource.GetAddressOf())
	);
	if (FAILED(hr)) {
		__debugbreak();
		goto RETURN;
	}

	if (_pInitImage) {
		D3D12_RESOURCE_DESC desc = pTexResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint = {};

		UINT rows = 0;
		UINT64 rowSize = 0;
		UINT64 TotalBytes = 0;
		// ���� �ؽ��� ���ҽ��� ������ ���� ���ҽ��鿡 ���� ���̾ƿ��� �����´�.
		// �ʺ�, ����, ����, RowPitch�� FOOTPRINT ����ü�� �޴´�.
		// ���� ��� ������ �� �𸣰ڴ�...
		m_pD3DDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footPrint, &rows, &rowSize, &TotalBytes);

		BYTE* pMappedPtr = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		// ������ ���ε� �ÿ� �ʿ��� ������ ũ�⸦ �����ִ� ����� �Լ��̴�.
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource.Get(), 0, 1);
		D3D12_RESOURCE_DESC resDesc_BuffSize = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		// upload buffer�� �����.
		hr = m_pD3DDevice->CreateCommittedResource(
			&heapProps_Upload,
			D3D12_HEAP_FLAG_NONE,
			&resDesc_BuffSize,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
		);
		if (FAILED(hr)) {
			__debugbreak();
			goto RETURN;
		}

		hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
		if (FAILED(hr)) {
			__debugbreak();
			goto RETURN;
		}
		// �Է����� ���� Texture ������ Upload Buffer�� �ø���
		const BYTE* pSrc = _pInitImage;
		BYTE* pDest = pMappedPtr;
		for (UINT y = 0; y < _height; y++) {
			memcpy(pDest, pSrc, _width * sizeof(uint32_t)); // �� �پ� �����Ѵ�.
			pSrc += (_width * sizeof(uint32_t));
			pDest += footPrint.Footprint.RowPitch;
			// RowPitch�� ���� ������ ����Ʈ ���̴�. (256 ����Ʈ Aligned�̴�.)
		}
		pUploadBuffer->Unmap(0, nullptr);

		UpdateTextureForWrite(pTexResource, pUploadBuffer);
	}

	*_ppOutResource = pTexResource;
RETURN:
	return hr;
}

HRESULT D3D12ResourceManager::CreateTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutResource, D3D12_RESOURCE_DESC* _pOutDesc, const WCHAR* _wchFileName)
{
	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3D12Resource> pTexResource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;
	UINT subresourceSize;
	UINT64 uploadBufferSize;
	D3D12_RESOURCE_DESC resDesc_BuffSize;

	// DirectXTex ���̺귯���� ����Ѵ�.
	hr = LoadDDSTextureFromFile(m_pD3DDevice.Get(), _wchFileName, &pTexResource, ddsData, subresourceData);
	if (FAILED(hr)) {
		goto RETURN;
	}

	// DirectXTex���� ����� ������ ������ Resource�� �����ؼ� GPU�� �ø���.
	textureDesc = pTexResource->GetDesc();
	subresourceSize = (UINT)subresourceData.size();
	uploadBufferSize = GetRequiredIntermediateSize(pTexResource.Get(), 0, subresourceSize);

	resDesc_BuffSize = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	hr = m_pD3DDevice->CreateCommittedResource(
		&HEAP_PROPS_UPLOAD,
		D3D12_HEAP_FLAG_NONE,
		&resDesc_BuffSize,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pUploadBuffer.GetAddressOf())
	);
	if (FAILED(hr)) {
		__debugbreak();
		goto RETURN;
	}

	if (FAILED(hr = m_pCommandAllocator->Reset())) {
		__debugbreak();
		goto RETURN;
	}
	if (FAILED(hr = m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
		goto RETURN;
	}

	{// GPU�� Resource�� �����Ѵ�.
		D3D12_RESOURCE_BARRIER barrier_SR_CPDest = CD3DX12_RESOURCE_BARRIER::Transition(pTexResource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		m_pCommandList->ResourceBarrier(1, &barrier_SR_CPDest);
		UpdateSubresources(m_pCommandList.Get(), pTexResource.Get(), pUploadBuffer.Get(), 0, 0, subresourceSize, &subresourceData[0]);
		D3D12_RESOURCE_BARRIER barrier_CPDest_SR = CD3DX12_RESOURCE_BARRIER::Transition(pTexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		m_pCommandList->ResourceBarrier(1, &barrier_CPDest_SR);

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	DoFence();
	WaitForFenceValue();

	*_ppOutResource = pTexResource;
	*_pOutDesc = textureDesc;

RETURN:
	return hr;
}

void D3D12ResourceManager::UpdateTextureForWrite(Microsoft::WRL::ComPtr<ID3D12Resource> _pDestTexResource, Microsoft::WRL::ComPtr<ID3D12Resource> _pSrcTexResource)
{
	// �������� ������ ���⿡ �ִ�.
	// https://learn.microsoft.com/ko-kr/windows/win32/direct3d12/upload-and-readback-of-texture-data


	// Texture�� GPU�� ���ε� �� ��, MipLevel ���� ���ε��Ѵ�.
	const DWORD MAX_SUB_RESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrints[MAX_SUB_RESOURCE_NUM];
	UINT rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64 rowSizes[MAX_SUB_RESOURCE_NUM] = {};
	UINT64 totalBytes = 0;

	D3D12_RESOURCE_DESC desc = _pDestTexResource->GetDesc();
	if (desc.MipLevels > static_cast<UINT>(_countof(footPrints))) {
		__debugbreak();
	}
	// ���ҽ� ������ ���
	m_pD3DDevice->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, footPrints, rows, rowSizes, &totalBytes);

	if (FAILED(m_pCommandAllocator->Reset())) {
		__debugbreak();
	}
	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
	}

	D3D12_RESOURCE_BARRIER texRB_SHADERRES_DEST = CD3DX12_RESOURCE_BARRIER::Transition(_pDestTexResource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	D3D12_RESOURCE_BARRIER texRB_DEST_SHADERRES = CD3DX12_RESOURCE_BARRIER::Transition(_pDestTexResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	m_pCommandList->ResourceBarrier(1, &texRB_SHADERRES_DEST);

	// ���ʴ�� GPU�� ���ε� �Ѵ�.
	for (DWORD i = 0; i < desc.MipLevels; i++) {
		// �ؽ��� ���縦 ���� ����ü�� ä���
		D3D12_TEXTURE_COPY_LOCATION destLocation = {};
		destLocation.PlacedFootprint = footPrints[i];
		destLocation.pResource = _pDestTexResource.Get();
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.PlacedFootprint = footPrints[i];
		srcLocation.pResource = _pSrcTexResource.Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		m_pCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
	m_pCommandList->ResourceBarrier(1, &texRB_DEST_SHADERRES);
	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	DoFence();
	WaitForFenceValue();	
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
