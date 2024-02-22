#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* _device, UINT _passCount, UINT _objectCount, UINT _materialCount)
{
	ThrowIfFailed(_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
	));
	// CB는 align이 필요하다.
	if (_passCount > 0)
	{
		PassCB = std::make_unique<UploadBuffer<PassConstants>>(_device, _passCount, true);
	}
	if (_objectCount > 0)
	{
		// InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(_device, _objectCount, false);

		// 다시 Object Constast Buffer로 돌아왔다.
		ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	}
	if (_materialCount > 0)
	{
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(_device, _materialCount, false);
	}
}

FrameResource::~FrameResource()
{
}
