#include "ScreenMap.h"

ScreenMap::ScreenMap(ID3D12Device* _device, UINT _width, UINT _height)
{
	m_d3dDevice = _device;

	m_Width = _width;
	m_Height = _height;

	m_Viewport = { 0.f, 0.f, (float)_width, (float)_height, 0.f, 1.f };
	m_ScissorRect = { 0, 0, (int)_width, (int)_height };

	BuildResource();
}

void ScreenMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDsv)
{
	// 맴버로 저장한다.
	m_hCpuSrv = _hCpuSrv;
	m_hGpuSrv = _hGpuSrv;
	m_hCpuRtv = _hCpuRtv;
	m_hCpuDsv = _hCpuDsv;

	BuildDescriptors();
}

void ScreenMap::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		// 크기에 맞춰 새로 텍스쳐를 만든다.
		BuildResource();

		BuildDescriptors();
	}
}

void ScreenMap::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	// Srv로 만들고
	m_d3dDevice->CreateShaderResourceView(m_ScreenMap.Get(), &srvDesc, m_hCpuSrv);
	// Rtv로도 만든다. 
	m_d3dDevice->CreateRenderTargetView(m_ScreenMap.Get(), nullptr, m_hCpuRtv);

	// 광원 시점에서 깊이버퍼를 작성하기 위한, Dsv을 만든다.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_dsvFormat; 
	dsvDesc.Texture2D.MipSlice = 0;
	m_d3dDevice->CreateDepthStencilView(m_ScreenDSBuffer.Get(), &dsvDesc, m_hCpuDsv);
}

void ScreenMap::BuildResource()
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
	const float* color = DirectX::Colors::LightSteelBlue;
	optClear.Color[0] = color[0];
	optClear.Color[1] = color[1];
	optClear.Color[2] = color[2];
	optClear.Color[3] = color[3];

	D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&m_ScreenMap)
	));

	// 새로운 카메라 해상도에 맞는 Depth-stencil buffer이다.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = m_dsvFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE dsOptClear;
	dsOptClear.Format = m_dsvFormat;
	dsOptClear.DepthStencil.Depth = 1.f;
	dsOptClear.DepthStencil.Stencil = 0;

	// 리소스를 만들고
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&dsOptClear,
		IID_PPV_ARGS(m_ScreenDSBuffer.GetAddressOf())
	));
}
