#pragma once

#include "../Common/d3dUtil.h"

class ScreenMap
{
public:
	ScreenMap(ID3D12Device* _device,
			  UINT _width, UINT _height);

	ScreenMap(const ScreenMap& _rhs) = delete;
	ScreenMap& operator=(const ScreenMap& _rhs) = delete;
	~ScreenMap() = default;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv, 
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
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenMap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_ScreenDSBuffer = nullptr;

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
		return m_ScreenMap.Get();
	}
	ID3D12Resource* GetResourceDS()
	{
		return m_ScreenDSBuffer.Get();
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv() const
	{
		return m_hGpuSrv;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv() const
	{
		return m_hCpuRtv;
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

