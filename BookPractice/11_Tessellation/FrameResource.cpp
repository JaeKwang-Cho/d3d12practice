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
		ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	}
	if (_materialCount > 0)
	{
		MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(_device, _materialCount, true);
	}
}

FrameResource::~FrameResource()
{
}
