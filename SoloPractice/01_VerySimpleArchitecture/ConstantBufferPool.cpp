// ConstantBufferPool.cpp from "megayuchi"

#include "pch.h"
#include "ConstantBufferPool.h"

bool ConstantBufferPool::Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, UINT _sizePerCBV, UINT _maxCBVNums)
{
	m_maxCBVNum = _maxCBVNums;
	m_sizePerCBV = _sizePerCBV;
	UINT byteWidth = m_sizePerCBV * m_maxCBVNum;

	// Constant Buffer를 엄청 크게 하나 만든다.
	D3D12_RESOURCE_DESC cbDesc_Size = CD3DX12_RESOURCE_DESC::Buffer(byteWidth);

	HRESULT hr = _pD3DDevice->CreateCommittedResource(
		&HEAP_PROPS_UPLOAD,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc_Size,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
	}

	// CBV가 올라갈 Descriptor Heap을 만든다.
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_maxCBVNum;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // CopyDescriptorsSimple() 을 사용하기 위해서 NONE으로 해야 한다.
	// 이후에 붙여넣기할 Descriptor가 SHADER_VISIBLE이면 된다.

	hr = _pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pCBVHeap));
	if (FAILED(hr)) {
		__debugbreak();
	}

	CD3DX12_RANGE writeRange(0, 0);
	m_pResource->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSystemMemAddr));

	// 그리고 Heap 위에 CBV를 여러개 만들어서 미리 가지고 있는다.
	m_pCBContainerList = new CB_CONTAINER[m_maxCBVNum];

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_sizePerCBV;

	UINT8* pSystemMemPtr = m_pSystemMemAddr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_pCBVHeap->GetCPUDescriptorHandleForHeapStart());

	UINT descriptorSize = _pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (UINT i = 0; i < m_maxCBVNum; i++) {
		_pD3DDevice->CreateConstantBufferView(&cbvDesc, heapHandle);
		// 이렇게 미리 값을 저장한다.
		m_pCBContainerList[i].cbvHandle = heapHandle;
		m_pCBContainerList[i].pGPUMemAddr = cbvDesc.BufferLocation;
		m_pCBContainerList[i].pSystemMemAddr = pSystemMemPtr;
		// heap 위에서 한칸식 밀면서 CBV를 생성한다.
		heapHandle.Offset(1, descriptorSize);
		cbvDesc.BufferLocation += m_sizePerCBV;
		pSystemMemPtr += m_sizePerCBV;
	}

	return true;
}

CB_CONTAINER* ConstantBufferPool::Alloc()
{
	// 요청하면 하나씩 꺼내서 준다.
	CB_CONTAINER* pCB = nullptr;

	if (m_allocatedCBVNum >= m_maxCBVNum)
		goto RETURN; // pool에 있는 모든 CB가 사용중이면 nullptr

	pCB = m_pCBContainerList + m_allocatedCBVNum;
	m_allocatedCBVNum++;
RETURN:
	return pCB;
}

void ConstantBufferPool::Reset()
{
	m_allocatedCBVNum = 0;
}

void ConstantBufferPool::CleanUpPool()
{
	if (m_pCBContainerList) {
		delete[] m_pCBContainerList;
		m_pCBContainerList = nullptr;
	}
}

ConstantBufferPool::ConstantBufferPool()
	:m_pCBContainerList(nullptr), m_pCBVHeap(nullptr), m_pResource(nullptr),
	m_pSystemMemAddr(nullptr), m_sizePerCBV(0), m_maxCBVNum(0), m_allocatedCBVNum(0)
{
}

ConstantBufferPool::~ConstantBufferPool()
{
	CleanUpPool();
}
