#include "pch.h"
#include "GameObject.h"
#include "Game.h"
#include "IRenderer.h"
#include "MeshGenerator.h"

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
		m_pRenderer->DrawRenderMesh(m_pMeshObj, &m_matWorld);
		m_pRenderer->DrawOutlineMesh(m_pMeshObj, &m_matWorld);
	}
}

IRenderMesh* GameObject::CreateBoxMeshObject()
{
	TextureMeshData meshData;
	std::vector<std::uint32_t> adjIndices;
	std::vector<SubmeshRange> ranges;

	CreateCube(1.f, 1.f, 1.f, meshData, adjIndices, ranges);

	IRenderMesh* pMeshObj = m_pRenderer->CreateTextureRenderMesh(meshData, adjIndices, ranges);
	if (pMeshObj == nullptr)
	{
		__debugbreak();
		return nullptr;
	}

	const WCHAR* wchTexFileNameList[6] =
	{
		L"tex_00.dds",
		L"tex_01.dds",
		L"tex_02.dds",
		L"tex_03.dds",
		L"tex_04.dds",
		L"tex_05.dds"
	};

	for (UINT i = 0; i < 6; i++)
	{
		TEXTURE_HANDLE* pTexture = reinterpret_cast<TEXTURE_HANDLE*>(m_pRenderer->CreateTextureFromFile(wchTexFileNameList[i]));
		m_pRenderer->BindTextureToMesh(pMeshObj, pTexture, i);

		CONSTANT_BUFFER_MATERIAL whiteMat(DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f));
		m_pRenderer->SetMeshMaterial(pMeshObj, whiteMat, i);
	}

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
		m_pRenderer->DeleteRenderMesh(m_pMeshObj);
		m_pMeshObj = nullptr;
	}
}

GameObject::~GameObject()
{
	Cleanup_GameObject();
}