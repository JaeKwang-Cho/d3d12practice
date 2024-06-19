// DescriptorPool.cpp from "megayuchi"

#include "pch.h"
#include "DescriptorPool.h"

bool DescriptorPool::Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, UINT _maxDescriptorCount)
{
	bool bResult = false;

	m_pD3DDevice = _pD3DDevice;
	m_MaxDescriptorCount = _maxDescriptorCount;
	m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Descriptor Heap을 여러개 만든다.
	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = m_MaxDescriptorCount;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr = m_pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pDescriptorHeap.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		goto RETURN;
	}
	m_cpuDescriptorHandle = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_gpuDescriptorHandle = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	bResult = true;
RETURN:
	return bResult;
}

bool DescriptorPool::AllocDescriptorTable(D3D12_CPU_DESCRIPTOR_HANDLE* _pOutCPUDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE* _pOutGPUDescriptor, UINT _descriptorCount)
{
	bool bResult = false;
	UINT offset = 0;
	if (m_AllocatedDescriptorCount + _descriptorCount > m_MaxDescriptorCount) {
		goto RETURN;
	}

	offset = m_AllocatedDescriptorCount + _descriptorCount;
	*_pOutCPUDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cpuDescriptorHandle, m_AllocatedDescriptorCount, m_srvDescriptorSize);
	*_pOutGPUDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_gpuDescriptorHandle, m_AllocatedDescriptorCount, m_srvDescriptorSize);
	m_AllocatedDescriptorCount += _descriptorCount;
	bResult = true;
RETURN:
	return bResult;
}

void DescriptorPool::Reset()
{
	m_AllocatedDescriptorCount = 0;
}

void DescriptorPool::CleanUpPool()
{
}

DescriptorPool::DescriptorPool()
	:m_pD3DDevice(nullptr), m_AllocatedDescriptorCount(0), m_MaxDescriptorCount(0), 
	m_srvDescriptorSize(0), m_pDescriptorHeap(nullptr), m_cpuDescriptorHandle{}, m_gpuDescriptorHandle{}
{
}

DescriptorPool::~DescriptorPool()
{
	CleanUpPool();
}
