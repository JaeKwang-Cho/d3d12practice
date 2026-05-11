#include "pch.h"
#include "GameObject.h"
#include "Game.h"
#include "D3D12Renderer.h"
#include "VertexUtil.h"

bool GameObject::Initialize_GameObject(Game* _pGame)
{
	m_pGame = _pGame;
	m_pRenderer = m_pGame->INL_GetRenderer();

	m_pMeshObj = CreateBoxMeshObject();
	if(m_pMeshObj == nullptr)
	{
		__debugbreak();
		return false;
	}

	return true;
}

void GameObject::SetGameObjectPosition(float _x, float _y, float _z)
{
	m_Pos.m128_f32[0] = _x;
	m_Pos.m128_f32[1] = _y;
	m_Pos.m128_f32[2] = _z;

	m_matTrans = XMMatrixTranslation(_x, _y, _z);

	m_bUpdateTransform = true;
}

void GameObject::SetGameObjectScale(float _x, float _y, float _z)
{
	m_Scale.m128_f32[0] = _x;
	m_Scale.m128_f32[1] = _y;
	m_Scale.m128_f32[2] = _z;

	m_matScale = XMMatrixScaling(_x, _y, _z);

	m_bUpdateTransform = true;
}

void GameObject::SetGameObjectRotationY(float _fRotY)
{
	m_fRotY = _fRotY;
	m_matRot = XMMatrixRotationY(_fRotY);
	m_bUpdateTransform = true;
}


void GameObject::Run()
{
	if(m_bUpdateTransform)
	{
		UpdateTransform();
		m_bUpdateTransform = false;
	}
}

void GameObject::Render()
{
	if(m_pMeshObj)
	{
		m_pRenderer->DrawRenderMesh(m_pMeshObj, &m_matWorld, E_RENDER_MESH_TYPE::TEXTURE);
	}
}

void* GameObject::CreateBoxMeshObject()
{
	void* pMeshObj = nullptr;
	// ±âº» Box
	uint16_t pIndexList[36] = {};
	ColorVertex* pVertexList = nullptr;
	ULONG dwVertexCount = CreateBoxMesh(&pVertexList, pIndexList, static_cast<DWORD>(_countof(pIndexList)), 0.25f);

	pMeshObj = m_pRenderer->CreateBasicMeshObject();

	const WCHAR* wchTexFileNameList[6] = 	{
		L"../../Assets/tex_00.dds",
		L"../../Assets/tex_01.dds",
		L"../../Assets/tex_02.dds",
		L"../../Assets/tex_03.dds",
		L"../../Assets/tex_04.dds",
		L"../../Assets/tex_05.dds"
	};

	m_pRenderer->BeginCreateMesh(pMeshObj, pVertexList, dwVertexCount, 6);
	for(ULONG i = 0 ; i < 6; ++i)
	{
		m_pRenderer->InsertTriGroup(pMeshObj, &pIndexList[i * 6], 2, wchTexFileNameList[i]);
	}
	m_pRenderer->EndCreateMesh(pMeshObj);

	DeleteBoxMesh(pVertexList);
	pVertexList = nullptr;

	return pVertexList;
}

void* GameObject::CreateQuadMeshObject()
{
	void* pMeshObj = nullptr;
	pMeshObj = m_pRenderer->CreateBasicMeshObject();

	// Set meshes to the BasicMeshObject
	ColorVertex pVertexList[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
	};

	WORD pIndexList[] =
	{
		0, 1, 2,
		0, 2, 3
	};


	m_pRenderer->BeginCreateMesh(pMeshObj, pVertexList, (DWORD)_countof(pVertexList), 1);
	m_pRenderer->InsertTriGroup(pMeshObj, pIndexList, 2, L"../../Assets/tex_06.dds");
	m_pRenderer->EndCreateMesh(pMeshObj);
	return pMeshObj;
}

void GameObject::UpdateTransform()
{
	// world = scale * rot * trans
	m_matWorld = m_matScale * m_matRot * m_matTrans;
}

GameObject::GameObject() :
	m_pGame(nullptr), m_pRenderer(nullptr), m_pMeshObj(nullptr), m_Scale(1.f, 1.f, 1.f, 0.f),
	m_Pos{}, m_fRotY(0.f), m_bUpdateTransform(false)
{
	m_matRot = XMMatrixIdentity();
	m_matScale = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();
}

void GameObject::Cleanup_GameObject()
{
	if (m_pMeshObj) {
		m_pRenderer->DeleteBasicMeshObject(m_pMeshObj);
		m_pMeshObj = nullptr;
	}
}

GameObject::~GameObject()
{
	Cleanup_GameObject();
}