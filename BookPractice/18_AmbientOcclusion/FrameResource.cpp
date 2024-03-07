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
		// 다시 Object Constast Buffer로 돌아왔다.
		ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	}

	// Screen 공간에 맞춰서 그리는 거니까 하나만 필요하다.
	SsaoCB = std::make_unique<UploadBuffer<SsaoConstants>>(_device, 1, true);

	if (_materialCount > 0)
	{
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(_device, _materialCount, false);
	}
}

FrameResource::~FrameResource()
{
}
