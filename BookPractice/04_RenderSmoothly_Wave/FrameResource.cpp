#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* _device, UINT _passCount, UINT _objectCount, UINT _waveVertexCount)
{
	ThrowIfFailed(_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
	));
	// CB는 aling이 필요하다.
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(_device, _passCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	// 얘는 Constant Buffer가 아니기 때문에 aling이 필요가 없다.
	WaveVB = std::make_unique<UploadBuffer<Vertex>>(_device, _waveVertexCount, false);
}

FrameResource::~FrameResource()
{
}
