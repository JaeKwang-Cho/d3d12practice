//***************************************************************************************
// RenderTarget.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

class RenderTarget
{
public:
	// �ӽ� Render Target ������ ���ִ� Ŭ���� �̴�.
	RenderTarget(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format);

	RenderTarget(const RenderTarget& _rhs) = delete;
	RenderTarget& operator=(const RenderTarget& _rhs) = delete;
	~RenderTarget() = default;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv);

	void OnResize(UINT _newWidth, UINT _newHeight);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* m_d3dDevice = nullptr;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv;

	// ����ۿ� ping - pong �� �� ���̴� �ؽ��� ���ҽ��̴�.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_OffscreenTex = nullptr;

public:
	// ������Ƽ
	ID3D12Resource* GetResource()
	{
		return m_OffscreenTex.Get();
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle()
	{
		return m_hGpuSrv;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtvHandle()
	{
		return m_hCpuRtv;
	}
};

