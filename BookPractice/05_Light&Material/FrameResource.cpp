#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* _device, UINT _passCount, UINT _objectCount, UINT _materialCount, UINT _waveVertexCount)
{
	ThrowIfFailed(_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
	));
	// CB�� align�� �ʿ��ϴ�.
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(_device, _passCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(_device, _objectCount, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(_device, _materialCount, true);

	// ��� Constant Buffer�� �ƴϱ� ������ align�� �ʿ䰡 ����.
	if (_waveVertexCount > 0)
	{
		WaveVB = std::make_unique<UploadBuffer<Vertex>>(_device, _waveVertexCount, false);
	}
}

FrameResource::~FrameResource()
{
}
