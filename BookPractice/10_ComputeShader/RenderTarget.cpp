//***************************************************************************************
// RenderTarget.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "RenderTarget.h"

RenderTarget::RenderTarget(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format)
{
	m_d3dDevice = _device;
	m_Width = _width;
	m_Height = _height;
	m_Format = _format;

	BuildResource();
}

void RenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv)
{
	m_hCpuSrv = _hCpuSrv;
	m_hGpuSrv = _hGpuSrv;
	m_hCpuRtv = _hCpuRtv;

	BuildDescriptors();
}

void RenderTarget::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		BuildResource();

		BuildDescriptors();
	}
}

void RenderTarget::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	// Srv로 만들고
	m_d3dDevice->CreateShaderResourceView(m_OffscreenTex.Get(), &srvDesc, m_hCpuSrv);
	// Rtv로도 만든다. 
	m_d3dDevice->CreateRenderTargetView(m_OffscreenTex.Get(), nullptr, m_hCpuRtv);
}

void RenderTarget::BuildResource()
{
	// 압축 포멧은 UAV로 사용할 수 없다.
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_Width;
	texDesc.Height = m_Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = m_Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = m_Format;
	optClear.Color[0] = 0.7f;
	optClear.Color[1] = 0.7f;
	optClear.Color[2] = 0.7f;
	optClear.Color[3] = 1.f;

	D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&m_OffscreenTex)
	));
}
