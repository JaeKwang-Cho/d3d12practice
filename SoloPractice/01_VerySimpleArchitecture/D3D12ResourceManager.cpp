// D3D12ResourceManager.cpp from "megayuchi"


#include "pch.h"
#include "D3D12ResourceManager.h"
#include "DirectXTexHeader.h"
#include "DDSTextureLoader12.h"

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

HRESULT D3D12ResourceManager::CreateIndexBuffer(DWORD _dwIndexNum, D3D12_INDEX_BUFFER_VIEW* _pOutIndexBufferView, Microsoft::WRL::ComPtr<ID3D12Resource>* _ppOutBuffer, void* _pInitData)
{
	HRESULT hr = S_OK;

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT indexBufferSize = sizeof(uint16_t) * _dwIndexNum;

	// Index도 upload heap buffer를 이용해서 default heap buffer에 데이터를 올린다.
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

		// Index 정보를 upload buffer에 올린다.
		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pIndexDataBegin));
		if (FAILED(hr)) {
			__debugbreak();
			goto RETURN;
		}
		memcpy(pIndexDataBegin, _pInitData, indexBufferSize);
		pUploadBuffer->Unmap(0, nullptr);

		// 이제 Upload Buffer에 있는 값을 Default 버퍼로 복사한다.
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

	// Index Buffer View를 채운다.
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

	// 텍스쳐도 마찬가지로 upload -> default를 이용해서 GPU에 올린다.
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
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, // 셰이더에서 사용하는 리소스는 그냥 이걸로 퉁치는 느낌
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
		// 현재 텍스쳐 리소스의 정보와 서브 리소스들에 대한 레이아웃을 가져온다.
		// 너비, 높이, 깊이, RowPitch를 FOOTPRINT 구조체에 받는다.
		// 아직 어디에 쓰는지 잘 모르겠다...
		m_pD3DDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footPrint, &rows, &rowSize, &TotalBytes);

		BYTE* pMappedPtr = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		// 데이터 업로드 시에 필요한 버퍼의 크기를 구해주는 도우미 함수이다.
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource.Get(), 0, 1);
		D3D12_RESOURCE_DESC resDesc_BuffSize = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		// upload buffer를 만든다.
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
		// 입력으로 들어온 Texture 정보를 Upload Buffer에 올린다
		const BYTE* pSrc = _pInitImage;
		BYTE* pDest = pMappedPtr;
		for (UINT y = 0; y < _height; y++) {
			memcpy(pDest, pSrc, _width * sizeof(uint32_t)); // 한 줄씩 복사한다.
			pSrc += (_width * sizeof(uint32_t));
			pDest += footPrint.Footprint.RowPitch;
			// RowPitch는 가로 한줄의 바이트 수이다. (256 바이트 Aligned이다.)
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

	// DirectXTex 라이브러리를 사용한다.
	hr = LoadDDSTextureFromFile(m_pD3DDevice.Get(), _wchFileName, &pTexResource, ddsData, subresourceData);
	if (FAILED(hr)) {
		goto RETURN;
	}

	// DirectXTex에서 얻어준 정보를 가지고 Resource를 생성해서 GPU에 올린다.
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

	{// GPU에 Resource를 생성한다.
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
	// 전반적인 내용은 여기에 있다.
	// https://learn.microsoft.com/ko-kr/windows/win32/direct3d12/upload-and-readback-of-texture-data


	// Texture를 GPU에 업로드 할 때, MipLevel 마다 업로드한다.
	const DWORD MAX_SUB_RESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrints[MAX_SUB_RESOURCE_NUM];
	UINT rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64 rowSizes[MAX_SUB_RESOURCE_NUM] = {};
	UINT64 totalBytes = 0;

	D3D12_RESOURCE_DESC desc = _pDestTexResource->GetDesc();
	if (desc.MipLevels > static_cast<UINT>(_countof(footPrints))) {
		__debugbreak();
	}
	// 리소스 정보를 얻고
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

	// 차례대로 GPU에 업로드 한다.
	for (DWORD i = 0; i < desc.MipLevels; i++) {
		// 텍스쳐 복사를 위한 구조체를 채우고
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
