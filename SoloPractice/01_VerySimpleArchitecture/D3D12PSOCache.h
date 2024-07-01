#pragma once
#include <unordered_map>

class D3D12Renderer;

class D3D12PSOCache
{
public:
	void Initialize(D3D12Renderer* _pRenderer);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPSO(std::string _strPSOName);
	bool CachePSO(std::string _strPSOName, Microsoft::WRL::ComPtr<ID3D12PipelineState> _pPSO);

protected:
private:
	void CleanUp();
public:
protected:
private:
	D3D12Renderer* m_pRenderer;
	// 일단은 string으로 한다.
	std::unordered_map <std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PSOMap;
public:
	D3D12PSOCache();
	~D3D12PSOCache();
};

