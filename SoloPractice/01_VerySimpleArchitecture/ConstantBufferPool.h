// ConstantBufferPool.h from "megayuchi"

#pragma once

// Object�� ������ �� �� ���� shader�� �Ѿ ģ����
struct CB_CONTAINER {
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
	D3D12_GPU_VIRTUAL_ADDRESS pGPUMemAddr; // table�� �ѱ�¿�
	UINT8* pSystemMemAddr; // ������Ʈ��
};

// �� Ŭ������ ������ �ϳ��� Object�� �� �����ӿ� ������ �ٸ��� ������ �ϱ� �����̴�.
// ���� 1: D3D12�� �񵿱��̱� ������ Drawcall�� �ϰ� CB���� �ٲ㼭 Drawcall�� �� �� ����
// ���� 2: CB�� ������ ������ �־ �����ϴ�. Drawcall�� �� �� DescriptorHeap�� Bind�� �Ѵ�.
// ��� : Constant Buffer View�� �������� �����, Drawcall ���� CBV�� Pool�� �ִ� heap �� �ϳ��� �����ؼ� bind�� ����.

class ConstantBufferPool
{
public:
	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, UINT _sizePerCBV, UINT _maxCBVNums);
	CB_CONTAINER* Alloc(); // �����ϸ� �ȵǴ� �����͸� ��ȯ�Ѵ�.
	void Reset();

protected:
private:
	void CleanUpPool();
public:
protected:
private:
	// �迭�� �����Ѵ�.
	CB_CONTAINER* m_pCBContainerList;
	// CBV�� �ö� Heap�̴�.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pCBVHeap;
	// CB resource
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
	// ���ε�� CPU memeory (typedef unsigned char UINT8)
	UINT8* m_pSystemMemAddr;
	// �ʱ�ȭ �� �� pool�� ��������� ������ ���Ѵ�.
	UINT m_sizePerCBV;
	UINT m_maxCBVNum;
	UINT m_allocatedCBVNum;
public:
	ConstantBufferPool();
	~ConstantBufferPool();
};

