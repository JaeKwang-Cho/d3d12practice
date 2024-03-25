//***************************************************************************************
// ShadowMap.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* _device,
			  UINT _width, UINT _height);

	ShadowMap(const ShadowMap& _rhs) = delete;
	ShadowMap& operator=(const ShadowMap& _rhs) = delete;
	~ShadowMap() = default;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDsv);

	void OnResize(UINT _newWidth, UINT _newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* m_d3dDevice = nullptr;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R24G8_TYPELESS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_ShadowMap = nullptr;

public:
	UINT GetWidth() const
	{
		return m_Width;
	}
	UINT GetHeight() const
	{
		return m_Height;
	}
	ID3D12Resource* GetResource()
	{
		return m_ShadowMap.Get();
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv() const
	{
		return m_hGpuSrv;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv() const
	{
		return m_hCpuDsv;
	}
	D3D12_VIEWPORT GetViewport() const
	{
		return m_Viewport;
	}
	D3D12_RECT GetScissorRect() const
	{
		return m_ScissorRect;
	}
};

