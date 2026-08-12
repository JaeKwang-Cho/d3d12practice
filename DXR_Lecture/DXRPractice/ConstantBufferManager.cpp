#include "pch.h"
#include "ConstantBufferManager.h"
#include "SimpleConstantBufferPool.h"

CONSTANT_BUFFER_PROPERTY f_pCBProperties[] = {
	{ CONSTANT_BUFFER_TYPE::DEFAULT, sizeof(CONSTANT_BUFFER_DEFAULT) },
	{ CONSTANT_BUFFER_TYPE::SPRITE, sizeof(CONSTANT_BUFFER_SPRITE) },
	{ CONSTANT_BUFFER_TYPE::RAY_TRACING, sizeof(CONSTANT_BUFFER_RAY_TRACING) }
};

bool ConstantBufferManager::Initialize(D3D12Device_raw _pD3DDevice, UINT _ulMaxCBVNum)
{
	for(UINT i = 0; i < static_cast<UINT>(CONSTANT_BUFFER_TYPE::COUNT); i++) {
		m_pConstantBufferPool[i] = std::make_unique<SimpleConstantBufferPool>();
		if (!m_pConstantBufferPool[i]->Initialize(_pD3DDevice, static_cast<CONSTANT_BUFFER_TYPE>(i), AlignConstantBufferSize(f_pCBProperties[i].cbSize), _ulMaxCBVNum)) {
			return false;
		}
	}
	return true;
}

void ConstantBufferManager::Reset_ConstantBufferManager()
{
	for (UINT i = 0; i < static_cast<UINT>(CONSTANT_BUFFER_TYPE::COUNT); i++) {
		if (m_pConstantBufferPool[i]) {
			m_pConstantBufferPool[i]->Reset_SimpleConstantBufferPool();
		}
	}
}

SimpleConstantBufferPool* ConstantBufferManager::GetConstantBufferPool(CONSTANT_BUFFER_TYPE _cbType)
{
	return m_pConstantBufferPool[static_cast<UINT>(_cbType)].get();
}

ConstantBufferManager::ConstantBufferManager()
{
}

ConstantBufferManager::~ConstantBufferManager()
{
}
