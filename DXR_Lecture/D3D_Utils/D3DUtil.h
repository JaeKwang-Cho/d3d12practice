// D3DUtil.h from "megayuchi"
#pragma once

void GetHardwareAdaptor(IDXGIFactory7* _pFactory, IDXGIAdapter4** _ppAdaptor);
void GetSoftwareAdaptor(IDXGIFactory7* _pFactory, IDXGIAdapter4** _ppAdaptor);
void SetDebugLayerInfo(ID3D12Device* _pD3DDevice);

// 자주	사용하는 sampler의 default 설정을 해주는 함수. 
// register index는 shader에서 사용할 때, register(s0) 이런식으로 지정하는 것이다.
void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* _pOutSamplerDesc, UINT _RegisterIndex);
void SetSamplerDesc_Wrap(D3D12_STATIC_SAMPLER_DESC* _pOutSamperDesc, UINT _RegisterIndex);
void SetSamplerDesc_Clamp(D3D12_STATIC_SAMPLER_DESC* _pOutSamperDesc, UINT _RegisterIndex);
void SetSamplerDesc_Border(D3D12_STATIC_SAMPLER_DESC* _pOutSamperDesc, UINT _RegisterIndex);
void SetSamplerDesc_Mirror(D3D12_STATIC_SAMPLER_DESC* _pOutSamperDesc, UINT _RegisterIndex);

void SerializeAndCreateRaytracingRootSignature(ID3D12Device* _pDevice, D3D12_ROOT_SIGNATURE_DESC* _pDesc, D3D12RootSignature_raw* _ppOutRootSig);

HRESULT CreateSimpleVertexBuffer(D3D12Device_ptr _pDevice, UINT _SizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView, D3D12Resource_ptr* _ppOutBuffer);

HRESULT CreateUploadBuffer(D3D12Device_raw _pDevice, void* _pData, UINT64 _DataSize, D3D12Resource_raw* _ppResource, const WCHAR* _wchResourceName);

inline size_t AlignConstantBufferSize(size_t _size) {
	// 이렇게 하면 256 보다 작은 값은 날라간다.
	size_t aligned_size = (_size + 255) & (~255);
	return aligned_size;
}

// alignment이 2의 제곱수라고 가정할 때, size를 alignment의 배수로 올림하여 반환하는 함수
inline UINT Align(UINT size, UINT alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

void UpdateTexture(D3D12Device_ptr _pD3DDevice, D3D12GraphicsCommandList_ptr _pCommandList, D3D12Resource_ptr _pDestTexResource, D3D12Resource_ptr _pSrcTexResource);
