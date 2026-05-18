#pragma once
class IRenderMesh
{
public:
	virtual ~IRenderMesh() = default;

	virtual void Draw(ULONG _ulThreadIndex, D3D12GraphicsCommandList_raw _pCommandList, const XMMATRIX* _pMatWorld) = 0;
	virtual void DrawOutline(ULONG _ulThreadIndex, D3D12GraphicsCommandList_raw _pCommandList, const XMMATRIX* _pMatWorld)
	{
		__debugbreak();
	}
	virtual bool SupportsOutline() const
	{
		return false;
	}
};

