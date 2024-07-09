// ConstantBufferManager.h from "megayuchi"

#pragma once

class ConstantBufferPool;

class ConstantBufferManager
{
public:
	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, DWORD _dwMaxCBVNum);
	void Reset();

	ConstantBufferPool* GetConstantBufferPool(E_CONSTANT_BUFFER_TYPE _type);
protected:
private:

public:
protected:
private:
	ConstantBufferPool* m_ppConstantBufferPool[(UINT)E_CONSTANT_BUFFER_TYPE::END];

public:
	ConstantBufferManager();
	~ConstantBufferManager();
};

