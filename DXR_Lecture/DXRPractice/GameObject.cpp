#include "pch.h"
#include <Windows.h>
#include <DirectXMath.h>
#include "../Util/VertexUtil.h"
#include "D3D12Renderer.h"
#include "Game.h"
#include "GameObject.h"

GameObject::GameObject()
{
	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();
}

GameObject::~GameObject()
{
	Cleanup_ITL();
}

BOOL GameObject::Initialize(Game* _pGame)
{
	BOOL bResult = FALSE;
	m_pGame = _pGame;
	m_pRenderer = _pGame->INL_GetRenderer();

	return bResult;
}

void GameObject::UpdateTransform_ITL()
{
	// world matrix = scale x rotation x translation
	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);

	if (m_pBlasHandle)
	{
		m_pRenderer->UpdateBLASTransform(m_pBlasHandle, &m_matWorld);
	}
	m_bUpdateTransform = FALSE;
}

void GameObject::SetPosition(float _x, float _y, float _z)
{
	m_Pos.m128_f32[0] = _x;
	m_Pos.m128_f32[1] = _y;
	m_Pos.m128_f32[2] = _z;

	m_matTrans = XMMatrixTranslation(_x, _y, _z);

	m_bUpdateTransform = TRUE;
}

void GameObject::SetScale(float _x, float _y, float _z)
{
	m_Scale.m128_f32[0] = _x;
	m_Scale.m128_f32[1] = _y;
	m_Scale.m128_f32[2] = _z;

	m_matScale = XMMatrixScaling(_x, _y, _z);

	m_bUpdateTransform = TRUE;
}

void GameObject::SetRotationY(float _fRotY)
{
	m_fRotY = _fRotY;
	m_matRot = XMMatrixRotationY(_fRotY);

	m_bUpdateTransform = TRUE;
}

void GameObject::Run()
{
	if (m_bUpdateTransform)
	{
		UpdateTransform_ITL();
	}
	else
	{
		int a = 0;
	}
}

void GameObject::Render()
{
	if (m_pMeshObj)
	{
		if (m_pRenderer->IsEnabledDXR())
		{
			// raytracing mode
		}
		else
		{
			// raster mode
			m_pRenderer->RenderMeshObject(m_pMeshObj, &m_matWorld);
		}
	}
}

void* GameObject::CreateBoxMeshObject()
{
	// create box mesh
	// create vertices and indices
	WORD pIndexList[36] = {};
	BasicVertex* pVertexList = nullptr;

	float fScale = (float)((rand() % 4) + 1);
	ULONG ulVertexCount = CreateBoxMesh(&pVertexList, pIndexList, (ULONG)_countof(pIndexList), 0.25f * fScale);

	// create BasicMeshObject from Renderer
	m_pMeshObj = m_pRenderer->CreateBasicMeshObject();

	const WCHAR* wchDiffuseTexFileNameList[6] =
	{
		L"tex_00.dds",
		L"tex_01.dds",
		L"tex_02.dds",
		L"tex_03.dds",
		L"tex_04.dds",
		L"tex_05.dds"
	};

	// Set meshes to the BasicMeshObject
	m_pRenderer->BeginCreateMesh(m_pMeshObj, pVertexList, ulVertexCount, 6);
	for (ULONG i = 0; i < 6; i++)
	{
		ULONG ulTexIndex = rand() % 6;
		m_pRenderer->InsertTriGroup(m_pMeshObj, pIndexList + i * 6, 2, wchDiffuseTexFileNameList[ulTexIndex]);
	}
	m_pRenderer->EndCreateMesh(m_pMeshObj);

	// delete vertices and indices
	if (pVertexList)
	{
		DeleteBoxMesh(pVertexList);
		pVertexList = nullptr;
	}
	if (m_pMeshObj)
	{
		m_pBlasHandle = m_pRenderer->CreateBLAS(m_pMeshObj);
	}
	return m_pMeshObj;
}

void* GameObject::CreateBottomMeshObject()
{
	// create bottom mesh
	// create vertices and indices
	WORD pIndexList[6] = {};
	BasicVertex pVertexList[4] = {};

	CreateBottomMesh(pVertexList, 4, pIndexList, 6, 10.0f, -1.0f);

	// create BasicMeshObject from Renderer
	m_pMeshObj = m_pRenderer->CreateBasicMeshObject();

	// Set meshes to the BasicMeshObject
	m_pRenderer->BeginCreateMesh(m_pMeshObj, pVertexList, 4, 1);
	m_pRenderer->InsertTriGroup(m_pMeshObj, pIndexList, 2, L"tilemap_008.dds");
	m_pRenderer->EndCreateMesh(m_pMeshObj);

	if (m_pMeshObj)
	{
		m_pBlasHandle = m_pRenderer->CreateBLAS(m_pMeshObj);
	}

	return m_pMeshObj;
}

void* GameObject::CreateQuadMesh()
{
	m_pMeshObj = m_pRenderer->CreateBasicMeshObject();

	// Set meshes to the BasicMeshObject
	BasicVertex pVertexList[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, {0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, {0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
	};

	WORD pIndexList[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	m_pRenderer->BeginCreateMesh(m_pMeshObj, pVertexList, (ULONG)_countof(pVertexList), 1);
	m_pRenderer->InsertTriGroup(m_pMeshObj, pIndexList, 2, L"tex_06.dds");
	m_pRenderer->EndCreateMesh(m_pMeshObj);

	if (m_pMeshObj)
	{
		m_pBlasHandle = m_pRenderer->CreateBLAS(m_pMeshObj);
	}
	return m_pMeshObj;
}

void GameObject::Cleanup_ITL()
{
	if (m_pMeshObj)
	{
		if (m_pBlasHandle)
		{
			m_pRenderer->DeleteBLAS(m_pMeshObj, m_pBlasHandle);
			m_pBlasHandle = nullptr;
		}
		m_pRenderer->DeleteBasicMeshObject(m_pMeshObj);
		m_pMeshObj = nullptr;
	}
}
