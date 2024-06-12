// D3DUtil.h from "megayuchi"
#pragma once

void GetHardwareAdaptor(IDXGIFactory7* _pFactory, IDXGIAdapter4** _ppAdaptor);
void GetSoftwareAdaptor(IDXGIFactory7* _pFactory, IDXGIAdapter4** _ppAdaptor);
void SetDebugLayerInfo(ID3D12Device* _pD3DDevice);
void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* _pOutSamplerDesc, UINT _RegisterIndex);
HRESULT CreateSimpleVertexBuffer(ID3D12Device* _pDevice, UINT _SizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView, ID3D12Resource** _ppOutBuffer);

inline size_t AlignConstantBufferSize(size_t _size) {
	// 이렇게 하면 256 보다 작은 값은 날라간다.
	size_t aligned_size = (_size + 255) & (~255);
	return aligned_size;
}

