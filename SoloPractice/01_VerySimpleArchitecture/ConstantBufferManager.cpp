// ConstantBufferManager.cpp from "megayuchi"

#include "pch.h"
#include "ConstantBufferManager.h"
#include "ConstantBufferPool.h"
#include "D3DUtil.h"

static CONSTANT_BUFFER_PROPERTY f_pCBPropList[] =
{
	{CONSTANT_BUFFER_TYPE::DEFAULT, sizeof(CONSTANT_BUFFER_DEFAULT)},
	{CONSTANT_BUFFER_TYPE::SPRITE, sizeof(CONSTANT_BUFFER_SPRITE)}
};

bool ConstantBufferManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, DWORD _dwMaxCBVNum)
{
	for (UINT i = 0; i < (UINT)CONSTANT_BUFFER_TYPE::END; i++) {
		m_ppConstantBufferPool[i] = new ConstantBufferPool;
		m_ppConstantBufferPool[i]->Initialize(_pD3DDevice, (CONSTANT_BUFFER_TYPE)i, (UINT)AlignConstantBufferSize(f_pCBPropList[i].size), _dwMaxCBVNum);
	}
	return true;
}

void ConstantBufferManager::Reset()
{
	for (UINT i = 0; i < (UINT)CONSTANT_BUFFER_TYPE::END; i++) {
		m_ppConstantBufferPool[i]->Reset();
	}
}

ConstantBufferPool* ConstantBufferManager::GetConstantBufferPool(CONSTANT_BUFFER_TYPE _type)
{
	if (_type >= CONSTANT_BUFFER_TYPE::END) {
		__debugbreak();
	}

	return m_ppConstantBufferPool[(UINT)_type];
}

ConstantBufferManager::ConstantBufferManager()
	:m_ppConstantBufferPool{}
{
}

ConstantBufferManager::~ConstantBufferManager()
{
	for (UINT i = 0; i < (UINT)CONSTANT_BUFFER_TYPE::END; i++) {
		if (m_ppConstantBufferPool[i]) {
			delete m_ppConstantBufferPool[i];
			m_ppConstantBufferPool[i] = nullptr;
		}
	}
}
