#include "pch.h"
#include "ShaderTable_Common.h"
#include "ShaderTable.h"
#include "RayTracingManager.h"

bool ShaderTable::Initialize(D3D12Device_raw _pD3DDevice, size_t _shaderRecordSize, const WCHAR* _wchResourceName)
{
	m_pD3DDevice = _pD3DDevice;
	// 32바이트 단위로 올림 한다.
	m_ShaderRecordSize = Align(static_cast<UINT>(_shaderRecordSize), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	wcscpy_s(m_wchResourceName, _wchResourceName);

	return true;
}

size_t ShaderTable::CommitResource(size_t _MaxShaderRecordCount)
{
	size_t MemorySize = _MaxShaderRecordCount * m_ShaderRecordSize;

	if (m_pResource) {
		m_pResource = nullptr;
	}

	CreateUploadBuffer(m_pD3DDevice, nullptr, MemorySize, m_pResource.GetAddressOf(), m_wchResourceName);
	if (!m_pResource) {
		__debugbreak();
		return 0;
	}

	// app이 종료될 때 까지, unmap을 하지 않는다. 
	CD3DX12_RANGE readRange(0, 0);
	HRESULT hr = m_pResource->Map(0, &readRange, (void**)&m_pMappedPtr);
	if (FAILED(hr)) {
		__debugbreak();
		return 0;
	}

	m_CurrShaderRecordCount = 0;
	m_pCurrWritePtr = m_pMappedPtr;
	m_ShaderRecordSize = m_ShaderRecordSize;
	m_MaxShaderRecordCount = _MaxShaderRecordCount;

	return m_MaxShaderRecordCount;
}

bool ShaderTable::InsertShaderRecord(const ShaderRecord* _pShaderRecord)
{
	if(m_CurrShaderRecordCount >= m_MaxShaderRecordCount)
	{
		__debugbreak();
		return false;
	}

	uint8_t* byteDest = static_cast<uint8_t*>(m_pCurrWritePtr);
	memcpy(byteDest, _pShaderRecord->shaderIdentifier.pPointer, _pShaderRecord->shaderIdentifier.size);
	if(_pShaderRecord->localRootArguments.pPointer)
	{
		memcpy(byteDest + _pShaderRecord->shaderIdentifier.size, _pShaderRecord->localRootArguments.pPointer, _pShaderRecord->localRootArguments.size);
	}

	m_pCurrWritePtr += m_ShaderRecordSize;
	m_CurrShaderRecordCount++;

	return true;
}

void RayTracingManager::BuildShaderTable()
{
}

void ShaderTable::CleanupShaderTable()
{

}

ShaderTable::ShaderTable()
{
}

ShaderTable::~ShaderTable()
{
	CleanupShaderTable();
}
