#pragma once

struct CB_CONTAINER 
{
	D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle;
	D3D12_GPU_VIRTUAL_ADDRESS pGPUMemAddress;
	UINT8* pSysMemAddress;
};

class SimpleConstantBufferPool
{
public:
	bool Initialize(D3D12Device_raw _pD3DDevice, CONSTANT_BUFFER_TYPE _cbType, UINT _uiSizePerCBV, UINT _uiMaxCBVNum);

	CB_CONTAINER* AllocCBContainer();
	void Reset_SimpleConstantBufferPool();
private:
	void Cleanup_SimpleConstantBufferPool();
private:
	std::unique_ptr<CB_CONTAINER[]> m_pCBContainerList = nullptr;
	CONSTANT_BUFFER_TYPE m_cbType;
	D3D12DescriptorHeap_ptr m_pCBVDescriptorHeap = nullptr;
	D3D12Resource_ptr m_pConstantBufferResource = nullptr;

	UINT8* m_pSystemMemAddr = nullptr;
	UINT m_SizePerCBV = 0;
	UINT m_MaxCBVNum = 0;
	UINT m_AllocatedCBVNum = 0;

public:
	SimpleConstantBufferPool();
	virtual ~SimpleConstantBufferPool();
};

