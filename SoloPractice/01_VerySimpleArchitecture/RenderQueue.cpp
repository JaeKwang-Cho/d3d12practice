#include "pch.h"
#include "RenderQueue.h"
#include "D3D12Renderer.h"
#include "IRenderMesh.h"
#include "SpriteObject.h"
#include "D3DUtil.h"

bool RenderQueue::Initialize(D3D12Renderer* _pRenderer, ULONG _ulMaxBufferSize)
{
	m_pRenderer = _pRenderer;

	m_RenderItemMaxCapacity = _ulMaxBufferSize;
	m_RenderItems.reserve(m_RenderItemMaxCapacity);
	m_RenderItems.resize(m_RenderItemMaxCapacity);

	return true;
}

bool RenderQueue::AddRenderItem(const RENDER_ITEM& _RenderItem)
{
	bool bResult = false;
	if(m_RenderItemCount + 1 > m_RenderItemMaxCapacity)
		goto EXIT;

	m_RenderItems[m_RenderItemCount] = _RenderItem;
	m_RenderItemCount++;
	bResult = true;

EXIT:
	return bResult;
}

ULONG RenderQueue::ProcessRenderItems(D3D12GraphicsCommandList_raw _pCommandList)
{
	D3D12Device_raw pDevice = m_pRenderer->INL_GetD3DDevice();

	ULONG uiCurrItemCount = 0;
	const RENDER_ITEM* pRenderItem = nullptr;
	while (pRenderItem = DispatchRenderItem()) {
		switch (pRenderItem->Type) {
			case RENDER_ITEM_TYPE::MESH:
			{
				IRenderMesh* pMesh = static_cast<IRenderMesh*>(pRenderItem->MeshObjParam.pMesh);
				switch (pRenderItem->MeshObjParam.Pass) {
					case RENDER_MESH_PASS::Default:
						pMesh->Draw(_pCommandList, &pRenderItem->MeshObjParam.matWorld);
						break;
					case RENDER_MESH_PASS::Outline:
						if (pMesh->SupportsOutline()) {
							pMesh->DrawOutline(_pCommandList, &pRenderItem->MeshObjParam.matWorld);
						}
						break;
					default:
						__debugbreak();
				}
			}break;
			case RENDER_ITEM_TYPE::SPRITE:
			{
				SpriteObject* pSprite = static_cast<SpriteObject*>(pRenderItem->SpriteParam.pSprite);

				XMFLOAT2 Pos = {
						static_cast<float>(pRenderItem->SpriteParam.iPosX),
						static_cast<float>(pRenderItem->SpriteParam.iPosY)
				};
				XMFLOAT2 Scale = {
					pRenderItem->SpriteParam.fScaleX,
					pRenderItem->SpriteParam.fScaleY
				};
				float Z = pRenderItem->SpriteParam.ZValue;

				TEXTURE_HANDLE* pTexHandle = static_cast<TEXTURE_HANDLE*>(pRenderItem->SpriteParam.pTexHandle);
				if (pTexHandle) {
					

					const RECT* pRect = nullptr;
					if (pRenderItem->SpriteParam.bUseRect)
						pRect = &pRenderItem->SpriteParam.Rect;

					if (pTexHandle->pUploadBuffer)
					{
						if(pTexHandle->bUpdated)
							UpdateTexture(pDevice, _pCommandList, pTexHandle->pTexResource, pTexHandle->pUploadBuffer);
						pTexHandle->bUpdated = false;
					}

					pSprite->DrawWithTex(_pCommandList, &Pos, &Scale, pRect, Z, pTexHandle);
				}
				else {
					pSprite->Draw(_pCommandList, &Pos, &Scale, Z);
				}
			}break;
			default:
				__debugbreak();
		}
		uiCurrItemCount++;
	}
	return uiCurrItemCount;
}

void RenderQueue::ResetQueue()
{
	m_RenderItemCount = 0;
	m_RenderItemReadIndex = 0;
}

RenderQueue::RenderQueue():
	m_pRenderer(nullptr),
	m_RenderItemMaxCapacity(0),
	m_RenderItemReadIndex(0),
	m_RenderItemCount(0)
{
}

RenderQueue::~RenderQueue()
{
	CleanupRenderItems();
}

const RENDER_ITEM* RenderQueue::DispatchRenderItem()
{
	const RENDER_ITEM* pRenderItem = nullptr;
	if(m_RenderItemReadIndex + 1 > m_RenderItemCount)
		goto EXIT;

	pRenderItem = (&m_RenderItems[m_RenderItemReadIndex]);
	m_RenderItemReadIndex++;

EXIT:
	return pRenderItem;
}

void RenderQueue::CleanupRenderItems()
{
	// STL vector는 자동으로 메모리를 관리하기 때문에 별도의 클린업이 필요하지 않다.
	// RenderItem의 포인터들도 외부에서 관리되고 있다고 가정하므로, RenderQueue에서는 해당 포인터들을 해제하지 않는다.
}
