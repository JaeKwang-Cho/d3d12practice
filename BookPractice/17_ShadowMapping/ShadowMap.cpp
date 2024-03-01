//***************************************************************************************
// ShadowMap.cpp by Frank Luna (C) 2011 All Rights Reserved.
//**************************************************************************************

#include "ShadowMap.h"

ShadowMap::ShadowMap(ID3D12Device* _device, UINT _width, UINT _height)
{
	m_d3dDevice = _device;

	m_Width = _width;
	m_Height = _height;

	m_Viewport = { 0.f, 0.f, (float)_width, (float)_height, 0.f, 0.f };
	m_ScissorRect = { 0, 0, (int)_width, (int)_height };

	BuildResource();
}

void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDsv)
{
	// �ɹ��� �����Ѵ�.
	m_hCpuSrv = _hCpuSrv;
	m_hGpuSrv = _hGpuSrv;
	m_hCpuDsv = _hCpuDsv;

	BuildDescriptors();
}

void ShadowMap::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		// ũ�⿡ ���� ���� �ؽ��ĸ� �����.
		BuildResource();

		BuildDescriptors();
	}
}

void ShadowMap::BuildDescriptors()
{
	// Srv�� ���� ���̴����� ����� �� �ֵ��� �Ѵ�.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // 24 ����ȭ ���� 8 Ÿ�Ը���
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	m_d3dDevice->CreateShaderResourceView(m_ShadowMap.Get(), &srvDesc, m_hCpuSrv);

	// ���� �������� ���̹��۸� �ۼ��ϱ� ����, Dsv�� �����.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //���� 24, ���ٽ� 8��Ʈ ���
	dsvDesc.Texture2D.MipSlice = 0;
	m_d3dDevice->CreateDepthStencilView(m_ShadowMap.Get(), &dsvDesc, m_hCpuSrv);
}

void ShadowMap::BuildResource()
{
	// ���� ���۸� ������ �ؽ��� ����
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
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&m_ShadowMap)
	));
}
