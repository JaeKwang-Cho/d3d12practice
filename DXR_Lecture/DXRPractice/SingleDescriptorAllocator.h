// SingleDescriptorAllocator.h from "megayuchi"

#pragma once

#include "../Utils/IndexCreator.h"

// Shader Resource View로 넘길 친구들을 하나의 Heap으로 관리한다.
// Descriptor Handle을 하나씩 빼서주고, 이걸 사용하는 호출자에서 Resource에 View를 만들어서 연결 시키는 것.
class SingleDescriptorAllocator
{
public:
	bool Initialize(D3D12Device_raw _pDevice, DWORD _dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS _flags);

	bool AllocDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE* _pOutCPUHandle);
	void FreeDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE _descriptorHandle);

	bool Check(D3D12_CPU_DESCRIPTOR_HANDLE _descriptorHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle);

protected:
private:
	void CleanUp();
public:
protected:
private:
	D3D12Device_raw m_pD3DDevice = nullptr;
	D3D12DescriptorHeap_ptr m_pHeap = nullptr;

	IndexCreator m_IndexCreator;
	UINT m_DescriptorSize = 0;

public:
	D3D12DescriptorHeap_ptr INL_GetDescriptorHeap() { return m_pHeap; }

	SingleDescriptorAllocator();
	~SingleDescriptorAllocator();
};

