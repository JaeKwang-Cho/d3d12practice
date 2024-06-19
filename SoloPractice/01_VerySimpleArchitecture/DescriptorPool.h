// DescriptorPool.h from "megayuchi"

#pragma once

// 이 클래스의 목적은 하나의 Object를 한 프레임에 여러번 다르게 렌더링 하기 위함이다.
// 이유 1: D3D12가 비동기이기 때문에 Drawcall을 하고 CB값을 바꿔서 Drawcall을 할 수 없음
// 이유 2: CB만 여러개 가지고 있어도 부족하다. Drawcall을 할 때 DescriptorHeap도 Bind를 한다.
// 방법 : Constant Buffer View를 여러개를 만들고, Drawcall 마다 CBV를 Pool에 있는 heap 중 하나에 복사해서 bind를 하자.

class DescriptorPool
{
public:
	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, UINT _maxDescriptorCount);
	bool AllocDescriptorTable(D3D12_CPU_DESCRIPTOR_HANDLE* _pOutCPUDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE* _pOutGPUDescriptor, UINT _descriptorCount);

	void Reset();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> INL_GetDescriptorHeap() { return m_pDescriptorHeap; }
protected:
private:
	void CleanUpPool();
public:
protected:
protected:
private:
	Microsoft::WRL::ComPtr<ID3D12Device14> m_pD3DDevice;

	UINT m_AllocatedDescriptorCount;
	UINT m_MaxDescriptorCount;
	UINT m_srvDescriptorSize;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandle;
public:
	DescriptorPool();
	~DescriptorPool();
};

