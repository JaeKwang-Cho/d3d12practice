#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* _device, UINT _passCount, UINT _objectCount, UINT _waveVertexCount)
{
	ThrowIfFailed(_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
	));
	// CB�� aling�� �ʿ��ϴ�.
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(_device, _passCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	// ��� Constant Buffer�� �ƴϱ� ������ aling�� �ʿ䰡 ����.
	WaveVB = std::make_unique<UploadBuffer<Vertex>>(_device, _waveVertexCount, false);
}

FrameResource::~FrameResource()
{
}
