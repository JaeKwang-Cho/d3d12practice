#pragma once

enum class RENDER_ITEM_TYPE
{
	MESH,
	SPRITE,
};

enum class RENDER_MESH_PASS
{
	Default,
	Outline,
};

struct RENDER_MESH_OBJ_PARAM
{
	class IRenderMesh* pMesh;
	XMMATRIX matWorld;
	RENDER_MESH_PASS Pass;
};

struct RENDER_SPRITE_PARAM
{
	int iPosX;
	int iPosY;

	float fScaleX;
	float fScaleY;

	RECT Rect;
	bool bUseRect;

	float ZValue;
	void* pTexHandle;
};

struct RENDER_ITEM
{	
	RENDER_ITEM_TYPE Type;
	void* pObjHandle;
	union {
		RENDER_MESH_OBJ_PARAM MeshObjParam;
		RENDER_SPRITE_PARAM SpriteParam;
	};
};

class D3D12Renderer;

class RenderQueue
{
public:
	bool Initialize(D3D12Renderer* _pRenderer, ULONG _ulMaxBufferSize);
	bool AddRenderItem(const RENDER_ITEM& _RenderItem);
	ULONG ProcessRenderItems(D3D12GraphicsCommandList_raw _pCommandList);

	void ResetQueue();

	RenderQueue();
	virtual ~RenderQueue();

private:
	const RENDER_ITEM* DispatchRenderItem();
	void CleanupRenderItems();

private:
	D3D12Renderer* m_pRenderer;
	std::vector<RENDER_ITEM> m_RenderItems;

	ULONG m_RenderItemMaxCapacity;
	ULONG m_RenderItemReadIndex;
	ULONG m_RenderItemCount;
};

