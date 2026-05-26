#pragma once

class ShaderRecord;

class ShaderTable
{
public:
	bool Initialize(D3D12Device_raw _pD3DDevice, size_t _shaderRecordSize, const WCHAR* _wchResourceName);
	size_t CommitResource(size_t _MaxShaderRecordCount);
	bool InsertShaderRecord(const ShaderRecord* _pShaderRecord);

	D3D12Resource_raw GetResource() const { return m_pResource.Get(); }

	size_t GetShaderRecordSize() const { return m_ShaderRecordSize;}
	size_t GetShaderRecordCount() const { return m_CurrShaderRecordCount;}
	size_t GetMaxShaderRecordCount() const { return m_MaxShaderRecordCount; }
	size_t GetHitGroupShaderTableSize() const { return m_ShaderRecordSize * m_CurrShaderRecordCount; }


private:
	void CleanupShaderTable();
private:
	D3D12Device_raw m_pD3DDevice = nullptr;
	D3D12Resource_ptr m_pResource = nullptr;

	uint8_t* m_pMappedPtr = nullptr;
	uint8_t* m_pCurrWritePtr = nullptr;

	size_t m_ShaderRecordSize = 0;
	size_t m_MaxShaderRecordCount = 0;
	size_t m_CurrShaderRecordCount = 0;

	WCHAR m_wchResourceName[128] = {};

public:
	ShaderTable();
	virtual ~ShaderTable();
};

