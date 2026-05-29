#pragma once

class SimpleConstantBufferPool;

class ConstantBufferManager
{
public:
	bool Initialize(D3D12Device_raw _pD3DDevice, ULONG _ulMaxCBVNum);
	void Reset_ConstantBufferManager();

	SimpleConstantBufferPool* GetConstantBufferPool(CONSTANT_BUFFER_TYPE _cbType);

private:
	std::unique_ptr<SimpleConstantBufferPool> m_pConstantBufferPool[static_cast<UINT>(CONSTANT_BUFFER_TYPE::COUNT)];
public:
	ConstantBufferManager();
	virtual~ConstantBufferManager();
};

