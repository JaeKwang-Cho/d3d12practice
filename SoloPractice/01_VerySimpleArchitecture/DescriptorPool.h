// DescriptorPool.h from "megayuchi"

#pragma once

// �� Ŭ������ ������ �ϳ��� Object�� �� �����ӿ� ������ �ٸ��� ������ �ϱ� �����̴�.
// ���� 1: D3D12�� �񵿱��̱� ������ Drawcall�� �ϰ� CB���� �ٲ㼭 Drawcall�� �� �� ����
// ���� 2: CB�� ������ ������ �־ �����ϴ�. Drawcall�� �� �� DescriptorHeap�� Bind�� �Ѵ�.
// ��� : Constant Buffer View�� �������� �����, Drawcall ���� CBV�� Pool�� �ִ� heap �� �ϳ��� �����ؼ� bind�� ����.

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

