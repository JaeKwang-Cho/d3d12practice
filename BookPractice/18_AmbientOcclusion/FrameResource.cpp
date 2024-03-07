#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* _device, UINT _passCount, UINT _objectCount, UINT _materialCount)
{
	ThrowIfFailed(_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
	));
	// CB�� align�� �ʿ��ϴ�.
	if (_passCount > 0)
	{
		PassCB = std::make_unique<UploadBuffer<PassConstants>>(_device, _passCount, true);
	}

	if (_objectCount > 0)
	{
		// �ٽ� Object Constast Buffer�� ���ƿԴ�.
		ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	}

	// Screen ������ ���缭 �׸��� �Ŵϱ� �ϳ��� �ʿ��ϴ�.
	SsaoCB = std::make_unique<UploadBuffer<SsaoConstants>>(_device, 1, true);

	if (_materialCount > 0)
	{
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(_device, _materialCount, false);
	}
}

FrameResource::~FrameResource()
{
}
