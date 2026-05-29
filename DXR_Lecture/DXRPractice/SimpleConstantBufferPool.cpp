#include "pch.h"
#include "SimpleConstantBufferPool.h"

bool SimpleConstantBufferPool::Initialize(D3D12Device_raw _pD3DDevice, CONSTANT_BUFFER_TYPE _cbType, UINT _uiSizePerCBV, UINT _uiMaxCBVNum)
{
	m_cbType = _cbType;
	m_MaxCBVNum = _uiMaxCBVNum;
	m_SizePerCBV = _uiSizePerCBV;

	UINT ByteWidth = m_SizePerCBV * m_MaxCBVNum;

	// Create CB resource
	auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffDesc = CD3DX12_RESOURCE_DESC::Buffer(ByteWidth);
	HRESULT hr = _pD3DDevice->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pConstantBufferResource.GetAddressOf())
	);

	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	// Create CBV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_MaxCBVNum;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = _pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pCBVDescriptorHeap.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	CD3DX12_RANGE readRange(0, 0);
	m_pConstantBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&m_pSystemMemAddr));

	m_pCBContainerList = std::make_unique<CB_CONTAINER[]>(m_MaxCBVNum);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pConstantBufferResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = ByteWidth;

	UINT8* pSystemMemAddr = m_pSystemMemAddr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_pCBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	UINT DescriptorSize = _pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for(UINT i = 0; i < m_MaxCBVNum; i++) {

		m_pCBContainerList[i].CBVHandle = heapHandle;
		m_pCBContainerList[i].pGPUMemAddress = cbvDesc.BufferLocation;
		m_pCBContainerList[i].pSysMemAddress = pSystemMemAddr;

		_pD3DDevice->CreateConstantBufferView(&cbvDesc, heapHandle);

		heapHandle.Offset(1, DescriptorSize);
		pSystemMemAddr += m_SizePerCBV;
		cbvDesc.BufferLocation += m_SizePerCBV;
	}

	return true;
}

CB_CONTAINER* SimpleConstantBufferPool::AllocCBContainer()
{
	CB_CONTAINER* pCBContainer = nullptr;
	if(m_AllocatedCBVNum >= m_MaxCBVNum) {
		__debugbreak();
		return nullptr;
	}
	pCBContainer = &m_pCBContainerList[m_AllocatedCBVNum++];

	return pCBContainer;
}

void SimpleConstantBufferPool::Reset_SimpleConstantBufferPool()
{
	m_AllocatedCBVNum = 0;
}

void SimpleConstantBufferPool::Cleanup_SimpleConstantBufferPool()
{
}

SimpleConstantBufferPool::SimpleConstantBufferPool():
	m_cbType(CONSTANT_BUFFER_TYPE::RAY_TRACING)
{
}

SimpleConstantBufferPool::~SimpleConstantBufferPool()
{
	Cleanup_SimpleConstantBufferPool();
}
