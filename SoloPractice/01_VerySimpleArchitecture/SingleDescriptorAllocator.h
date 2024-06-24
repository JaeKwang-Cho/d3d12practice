// SingleDescriptorAllocator.h from "megayuchi"

#pragma once

#include "IndexCreator.h"

// Shader Resource View�� �ѱ� ģ������ �ϳ��� Heap���� �����Ѵ�.
// Descriptor Handle�� �ϳ��� �����ְ�, �̰� ����ϴ� ȣ���ڿ��� Resource�� View�� ���� ���� ��Ű�� ��.
class SingleDescriptorAllocator
{
public:
	bool Initialize(Microsoft::WRL::ComPtr< ID3D12Device14> _pDevice, DWORD _dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS _flags);

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
	Microsoft::WRL::ComPtr< ID3D12Device14> m_pD3DDevice;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pHeap;

	IndexCreator m_IndexCreator;
	UINT m_DescriptorSize = 0;

public:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> INL_GetDescriptorHeap() { return m_pHeap; }

	SingleDescriptorAllocator();
	~SingleDescriptorAllocator();
};

