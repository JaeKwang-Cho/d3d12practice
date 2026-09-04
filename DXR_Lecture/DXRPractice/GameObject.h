#pragma once

#ifdef __INTELLISENSE__
#include "pch.h"
#endif

class Game;
class D3D12Renderer;

class GameObject
{
public:
	bool Initialize(Game* _pGame);
	void* CreateBoxMeshObject();
	void* CreateQuadMesh();
	void* CreateBottomMeshObject();
	void SetPosition(float _x, float _y, float _z);
	void SetScale(float _x, float _y, float _z);
	void SetRotationY(float _fRotY);
	void Run();
	void Render();

	GameObject();
	virtual ~GameObject();

private:
	void UpdateTransform_ITL();
	void Cleanup_ITL();

private:
	Game* m_pGame = nullptr;
	D3D12Renderer* m_pRenderer = nullptr;
	void* m_pMeshObj = nullptr;
	void* m_pBlasHandle = nullptr;

	XMVECTOR m_Scale = { 1.0f, 1.0f, 1.0f, 0.0f };
	XMVECTOR m_Pos = {};
	float m_fRotY = 0.0f;

	XMMATRIX m_matScale = {};
	XMMATRIX m_matRot = {};
	XMMATRIX m_matTrans = {};
	XMMATRIX m_matWorld = {};
	bool m_bUpdateTransform = false;
};

