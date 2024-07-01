#include "pch.h"
#include "typedef.h"
#include "D3D12PSOCache.h"
#include "D3D12Renderer.h"

void D3D12PSOCache::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PSOCache::GetPSO(std::string _strPSOName)
{
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPSO = nullptr;
	auto iter = m_PSOMap.find(_strPSOName);
	if (iter != m_PSOMap.end()) {
		pPSO = iter->second;
	}
	return pPSO;
}

bool D3D12PSOCache::CachePSO(std::string _strPSOName, Microsoft::WRL::ComPtr<ID3D12PipelineState> _pPSO)
{
	auto iter = m_PSOMap.find(_strPSOName);
	if (iter != m_PSOMap.end()) {
		__debugbreak();
		return false;
	}

	m_PSOMap.insert({ _strPSOName, _pPSO });
	return true;
}

void D3D12PSOCache::CleanUp()
{
}

D3D12PSOCache::D3D12PSOCache()
	:m_pRenderer(nullptr), m_PSOMap{}
{
}

D3D12PSOCache::~D3D12PSOCache()
{
}
