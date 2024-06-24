// SingleDescriptorAllocator.h from "megayuchi"

#include "pch.h"
#include "SingleDescriptorAllocator.h"

bool SingleDescriptorAllocator::Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pDevice, DWORD _dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS _flags)
{
	m_pD3DDevice = _pDevice;

	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = _dwMaxCount;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = _flags; // D3D12_DESCRIPTOR_FLAG_NONE

	HRESULT hr = m_pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pHeap.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	m_IndexCreator.Initialize(_dwMaxCount);
	m_DescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}

bool SingleDescriptorAllocator::AllocDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE* _pOutCPUHandle)
{
	bool bResult = false;
	
	DWORD dwIndex = m_IndexCreator.Alloc();
	if (-1 != dwIndex) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_pHeap->GetCPUDescriptorHandleForHeapStart(), dwIndex, m_DescriptorSize);
		*_pOutCPUHandle = descriptorHandle;
		bResult = true;
	}

	return bResult;
}

void SingleDescriptorAllocator::FreeDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE _descriptorHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE hBase = m_pHeap->GetCPUDescriptorHandleForHeapStart();
#ifdef _DEBUG
	if (hBase.ptr > _descriptorHandle.ptr) {
		__debugbreak();
	}
#endif
	// 인자로 들어온 Handle의 offset으로 index를 구해, IndexCreate 클래스에서도 해제해준다.
	DWORD dwIndex = (DWORD)(_descriptorHandle.ptr - hBase.ptr) / m_DescriptorSize;
	m_IndexCreator.Free(dwIndex);
}

bool SingleDescriptorAllocator::Check(D3D12_CPU_DESCRIPTOR_HANDLE _descriptorHandle)
{
	bool bResult = true;
	
	D3D12_CPU_DESCRIPTOR_HANDLE hBase = m_pHeap->GetCPUDescriptorHandleForHeapStart();
#ifdef _DEBUG
	if (hBase.ptr > _descriptorHandle.ptr) {
		__debugbreak();
		bResult = false;
	}
#endif 
	return bResult;
}

D3D12_GPU_DESCRIPTOR_HANDLE SingleDescriptorAllocator::GetGPUHandleFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE hBase = m_pHeap->GetCPUDescriptorHandleForHeapStart();
#ifdef _DEBUG
	if (hBase.ptr > _cpuHandle.ptr) {
		__debugbreak();
	}
#endif
	// 시작지점을 기준으로 offset을 구해서, 대응하는 GPU 핸들 위치를 찾는다.
	DWORD dwIndex = (DWORD)(_cpuHandle.ptr - hBase.ptr) / m_DescriptorSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_pHeap->GetGPUDescriptorHandleForHeapStart(), dwIndex, m_DescriptorSize);

	return gpuHandle;
}

void SingleDescriptorAllocator::CleanUp()
{
#ifdef _DEBUG
	m_IndexCreator.Check();
#endif
}

SingleDescriptorAllocator::SingleDescriptorAllocator()
	:m_pD3DDevice(nullptr), m_pHeap(nullptr), m_DescriptorSize(0)
{
}

SingleDescriptorAllocator::~SingleDescriptorAllocator()
{
	CleanUp();
}
