//***************************************************************************************
// CubeRenderTarget.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

enum class ECubeMapFace : int
{
	PositiveX = 0,
	NegativeX = 1,
	PositiveY = 2,
	NegativeY = 3,
	PositiveZ = 4,
	NegativeZ = 5
};

class CubeRenderTarget
{
public:
	CubeRenderTarget(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format);
	CubeRenderTarget(const CubeRenderTarget& _other) = delete;
	CubeRenderTarget& operator=(const CubeRenderTarget& _other) = delete;
	~CubeRenderTarget() = default;

	void BuildDescriptor(
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv[6]
	);

	void OnResize(UINT _newWidth, UINT _newHeight);
private:
	void BuildResource();
	void BuildDescriptor();

private:
	ID3D12Device* m_d3dDevice = nullptr;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv[6];

	Microsoft::WRL::ComPtr<ID3D12Resource> m_CubeMap = nullptr;

public:
	ID3D12Resource* GetResource()
	{
		return m_CubeMap.Get();
	}
	
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv()
	{
		return m_hGpuSrv;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int _faceIndex)
	{
		return m_hCpuRtv[_faceIndex];
	}

	D3D12_VIEWPORT GetViewport()
	{
		return m_Viewport;
	}

	D3D12_RECT GetScissorRect()
	{
		return m_ScissorRect;
	}
};

