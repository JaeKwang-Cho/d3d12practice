//***************************************************************************************
// SobelFilter.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "SobelFilter.h"

SobelFilter::SobelFilter(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format)
{
	m_d3dDevice = _device;
	m_Width = _width;
	m_Height = _height;
	m_Format = _format;

	BuildResource();
}

void SobelFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor, UINT _descriptorSize)
{
	m_hCpuSrv = _hCpuDescriptor;
	m_hCpuUav = _hCpuDescriptor.Offset(1, _descriptorSize);
	m_hGpuSrv = _hGpuDescriptor;
	m_hGpuUav = _hGpuDescriptor.Offset(1, _descriptorSize);

	BuildDescriptors();
}

void SobelFilter::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		BuildResource();

		BuildDescriptors();
	}
}

void SobelFilter::Execute(ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _pso, CD3DX12_GPU_DESCRIPTOR_HANDLE _input)
{
	_cmdList->SetComputeRootSignature(_rootSig);
	_cmdList->SetPipelineState(_pso);

	_cmdList->SetComputeRootDescriptorTable(0, _input);
	_cmdList->SetComputeRootDescriptorTable(2, m_hGpuUav);

	D3D12_RESOURCE_BARRIER output_READ_UA = CD3DX12_RESOURCE_BARRIER::Transition(m_Output.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	_cmdList->ResourceBarrier(1, &output_READ_UA);

	UINT numGroupsX = (UINT)ceilf(m_Width / 16.0f);
	UINT numGroupsY = (UINT)ceilf(m_Height / 16.0f);
	_cmdList->Dispatch(numGroupsX, numGroupsY, 1);

	D3D12_RESOURCE_BARRIER output_UA_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_Output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
	_cmdList->ResourceBarrier(1, &output_UA_READ);
}

void SobelFilter::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = m_Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	m_d3dDevice->CreateShaderResourceView(m_Output.Get(), &srvDesc, m_hCpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_Output.Get(), nullptr, &uavDesc, m_hCpuUav);
}

void SobelFilter::BuildResource()
{
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
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Output)));
}
