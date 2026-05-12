#include "pch.h"
#include "GameObject.h"
#include "Game.h"
#include "D3D12Renderer.h"
#include "VertexUtil.h"
#include "TextureRenderMesh.h"

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
		m_pRenderer->DrawOutlineMesh(m_pMeshObj, &m_matWorld);
	}
}

void* GameObject::CreateBoxMeshObject()
{
	TextureRenderMesh* pNewBox = nullptr;
	// ±âº» Box
	TextureMeshData meshData;
	std::vector<uint32_t> AdjIndices;
	std::vector<SubmeshRange> Ranges;

	CreateCube(1.f, 1.f, 1.f, meshData, AdjIndices, Ranges);

	pNewBox = new TextureRenderMesh;
	pNewBox->Initialize(m_pRenderer);

	if(!pNewBox->CreateRenderAssetsFromSingleMesh(meshData, AdjIndices, Ranges))
	{
		delete pNewBox;
		__debugbreak();
		return nullptr;
	}

	const WCHAR* wchTexFileNameList[6] = {
		L"../../Assets/tex_00.dds",
		L"../../Assets/tex_01.dds",
		L"../../Assets/tex_02.dds",
		L"../../Assets/tex_03.dds",
		L"../../Assets/tex_04.dds",
		L"../../Assets/tex_05.dds"
	};

	for (UINT i = 0; i < 6; i++)
	{
		TEXTURE_HANDLE* pTexture = reinterpret_cast<TEXTURE_HANDLE*>(m_pRenderer->CreateTextureFromFile(wchTexFileNameList[i]));
		pNewBox->BindTextureAssets(pTexture, i);
		CONSTANT_BUFFER_MATERIAL whiteMat = CONSTANT_BUFFER_MATERIAL(XMFLOAT4(1.f, 1.f, 1.f, 1.f));
		pNewBox->SetMaterial(whiteMat, i);
	}

	return pNewBox;
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
		TextureRenderMesh* pMesh = reinterpret_cast<TextureRenderMesh*>(m_pMeshObj);
		delete pMesh;
		m_pMeshObj = nullptr;
	}
}

GameObject::~GameObject()
{
	Cleanup_GameObject();
}