//***************************************************************************************
// SobelFilter.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Applies a sobel filter on the topmost mip level of an input texture.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

class SobelFilter
{
public:
	SobelFilter(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format);

	SobelFilter(const SobelFilter& rhs) = delete;
	SobelFilter& operator=(const SobelFilter& rhs) = delete;
	~SobelFilter() = default;

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor, UINT _descriptorSize);

	void OnResize(UINT _newWidth, UINT _newHeight);

	void Execute(ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _pso, CD3DX12_GPU_DESCRIPTOR_HANDLE _input);

private:
	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* m_d3dDevice = nullptr;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuUav;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_Output = nullptr;

public:
	// 프로퍼티
	CD3DX12_GPU_DESCRIPTOR_HANDLE OutputSrv()
	{
		return m_hGpuSrv;
	}

	UINT GetDescriptorCount()const
	{
		return 2;
	}
};

