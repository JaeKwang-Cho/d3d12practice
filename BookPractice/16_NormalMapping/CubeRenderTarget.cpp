//***************************************************************************************
// CubeRenderTarget.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "CubeRenderTarget.h"

CubeRenderTarget::CubeRenderTarget(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format)
{
	m_d3dDevice = _device;

	m_Width = _width;
	m_Height = _height;
	m_Format = _format;

	m_Viewport = { 0.f, 0.f ,(float)_width, (float)_height, 0.f, 1.f };
	m_ScissorRect = { 0, 0, (int)_width, (int)_height };

	BuildResource();
}

void CubeRenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv[6])
{
	m_hCpuSrv = _hCpuSrv;
	m_hGpuSrv = _hGpuSrv;
	
	for (int i = 0; i < 6; i++)
	{
		m_hCpuRtv[i] = _hCpuRtv[i];
	}

	BuildDescriptor();
}

void CubeRenderTarget::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		BuildResource();

		BuildDescriptor();
	}
}

void CubeRenderTarget::BuildResource()
{
	// 렌더 타겟용, 텍스쳐 6개 짜리 리소스를 만든다.
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_Width;
	texDesc.Height = m_Height;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = 1;
	texDesc.Format = m_Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = m_Format;
	const float* color = DirectX::Colors::LightSteelBlue;
	optClear.Color[0] = color[0];
	optClear.Color[1] = color[1];
	optClear.Color[2] = color[2];
	optClear.Color[3] = color[3];

	CD3DX12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&m_CubeMap)
	));
}


void CubeRenderTarget::BuildDescriptor()
{
	// 쉐이더에서 사용할 TextureCube이다.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // 큐브맵을
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.f;

	m_d3dDevice->CreateShaderResourceView(m_CubeMap.Get(), &srvDesc, m_hCpuSrv);

	// cube의 각 맵을 구성하는 랜더타겟 6개이다.
	for (int i = 0; i < 6; i++)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		// TEXTURE2DARRAY로 만들고
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY; 
		rtvDesc.Format = m_Format;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		// 각 뷰마다 렌더 타겟 번호를 지정해준다.
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		rtvDesc.Texture2DArray.ArraySize = 1;
		
		m_d3dDevice->CreateRenderTargetView(m_CubeMap.Get(), &rtvDesc, m_hCpuRtv[i]);
	}
}

