//***************************************************************************************
// Ssao.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Ssao.h"

Ssao::Ssao(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, UINT _width, UINT _height)
{
	m_d3dDevice = _device;

	OnResize(_width, _height);

	BuildOffsetVectors();
	BuildRandomVectorTexture(_cmdList);
}

void Ssao::GetOffsetVectors(DirectX::XMFLOAT4 _offsets[14])
{
	std::copy(&m_Offsets[0], &m_Offsets[14], &_offsets[0]);
}

std::vector<float> Ssao::CalcGaussWeights(float _sigma)
{
	return std::vector<float>();
}

void Ssao::BuildDescriptors(ID3D12Resource* _depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv, UINT _cbvSrvUavDescriptorSize, UINT _rtvDescriptorSize)
{
}

void Ssao::RebuildDescriptors(ID3D12Resource* _depthStencilBuffer)
{
}

void Ssao::SetPSOs(ID3D12PipelineState* _ssaoPso, ID3D12PipelineState* _ssaoBlurPso)
{
}

void Ssao::OnResize(UINT _newWidth, UINT _newHeight)
{
}

void Ssao::ComputeSsao(ID3D12GraphicsCommandList* _cmdList, FrameResource* _currFrame, int _blurCount)
{
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, FrameResource* _currFrame, int _blurCount)
{
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, bool _horzBlur)
{
}

void Ssao::BuildResources()
{
}

void Ssao::BuildRandomVectorTexture(ID3D12GraphicsCommandList* _cmdList)
{
}

void Ssao::BuildOffsetVectors()
{
}
