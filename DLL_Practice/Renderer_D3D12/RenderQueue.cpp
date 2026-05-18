#include "pch.h"
#include "RenderQueue.h"
#include "D3D12Renderer.h"
#include "IRenderMesh.h"
#include "SpriteObject.h"
#include "D3DUtil.h"
#include "CommandListPool.h"

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

ULONG RenderQueue::ProcessRenderItems(ULONG _ulThreadIndex, CommandListPool* _pCommandListPool, D3D12CommandQueue_raw _pCommandQueue, ULONG _ulProcessCountPerCommandList, D3D12_CPU_DESCRIPTOR_HANDLE _rtv, D3D12_CPU_DESCRIPTOR_HANDLE _dsv, const D3D12_VIEWPORT* _pViewport, const D3D12_RECT* _pScissorRect)
{
	D3D12Device_raw pDevice = m_pRenderer->INL_GetD3DDevice();

	D3D12GraphicsCommandList_raw ppCommandList[64] = {};
	ULONG ulCommandListCount = 0;

	D3D12GraphicsCommandList_raw pCommandList = nullptr;
	ULONG uiProcessedItemCount = 0;
	ULONG uiProcessedItemCountPerCommandList = 0;

	const RENDER_ITEM* pRenderItem = nullptr;

	while (pRenderItem = DispatchRenderItem()) {
		pCommandList = _pCommandListPool->GetCurrentCommandList();
		// viewport, scissor rect, render target을 설정해줘야
		// 그 위에 뭔가를 그릴 수 있다.
		pCommandList->RSSetViewports(1, _pViewport);
		pCommandList->RSSetScissorRects(1, _pScissorRect);
		// 이제 z버퍼를 함께 넣어준다.
		pCommandList->OMSetRenderTargets(1, &_rtv, FALSE, &_dsv);

		ProcessRenderItem_ITL(_ulThreadIndex, pCommandList, pRenderItem);

		uiProcessedItemCount++;
		uiProcessedItemCountPerCommandList++;
		if (uiProcessedItemCountPerCommandList >= _ulProcessCountPerCommandList)
		{
			_pCommandListPool->CloseCurrentCommandList();
			ppCommandList[ulCommandListCount] = pCommandList;
			ulCommandListCount++;
			pCommandList = nullptr;
			uiProcessedItemCountPerCommandList = 0;
		}
	}

	if(uiProcessedItemCountPerCommandList)
	{
		_pCommandListPool->CloseCurrentCommandList();
		ppCommandList[ulCommandListCount] = pCommandList;
		ulCommandListCount++;
	}
	if (ulCommandListCount) {
		_pCommandQueue->ExecuteCommandLists(ulCommandListCount, (ID3D12CommandList**)ppCommandList);
	}
	m_RenderItemCount = 0;
	return uiProcessedItemCount;
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

void RenderQueue::ProcessRenderItem_ITL(ULONG _ulThreadIndex, D3D12GraphicsCommandList_raw _pCommandList, const RENDER_ITEM* _pRenderItem)
{
	D3D12Device_raw pDevice = m_pRenderer->INL_GetD3DDevice();

	switch (_pRenderItem->Type) {
	case RENDER_ITEM_TYPE::MESH:
	{
		IRenderMesh* pMesh = static_cast<IRenderMesh*>(_pRenderItem->MeshObjParam.pMesh);
		switch (_pRenderItem->MeshObjParam.Pass) {
		case RENDER_MESH_PASS::Default:
			pMesh->Draw(_ulThreadIndex, _pCommandList, &_pRenderItem->MeshObjParam.matWorld);
			break;
		case RENDER_MESH_PASS::Outline:
			if (pMesh->SupportsOutline()) {
				pMesh->DrawOutline(_ulThreadIndex, _pCommandList, &_pRenderItem->MeshObjParam.matWorld);
			}
			break;
		default:
			__debugbreak();
		}
	}break;
	case RENDER_ITEM_TYPE::SPRITE:
	{
		SpriteObject* pSprite = static_cast<SpriteObject*>(_pRenderItem->SpriteParam.pSprite);

		XMFLOAT2 Pos = {
				static_cast<float>(_pRenderItem->SpriteParam.iPosX),
				static_cast<float>(_pRenderItem->SpriteParam.iPosY)
		};
		XMFLOAT2 Scale = {
			_pRenderItem->SpriteParam.fScaleX,
			_pRenderItem->SpriteParam.fScaleY
		};
		float Z = _pRenderItem->SpriteParam.ZValue;

		TEXTURE_HANDLE* pTexHandle = static_cast<TEXTURE_HANDLE*>(_pRenderItem->SpriteParam.pTexHandle);
		if (pTexHandle) {


			const RECT* pRect = nullptr;
			if (_pRenderItem->SpriteParam.bUseRect)
				pRect = &_pRenderItem->SpriteParam.Rect;

			if (pTexHandle->pUploadBuffer)
			{
				if (pTexHandle->bUpdated)
					UpdateTexture(pDevice, _pCommandList, pTexHandle->pTexResource, pTexHandle->pUploadBuffer);
				pTexHandle->bUpdated = false;
			}

			pSprite->DrawWithTex(_ulThreadIndex	, _pCommandList, &Pos, &Scale, pRect, Z, pTexHandle);
		}
		else {
			pSprite->Draw(_ulThreadIndex, _pCommandList, &Pos, &Scale, Z);
		}
	}break;
	default:
		__debugbreak();
	}
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
