#pragma once

class Game;
class D3D12Renderer;

class GameObject
{
public:
	bool Initialize_GameObject(Game* _pGame);

	void SetGameObjectPosition(float _x, float _y, float _z);
	void SetGameObjectScale(float _x, float _y, float _z);
	void SetGameObjectRotationY(float _fRotY);

	void Run();
	void Render();

	GameObject();
	virtual ~GameObject();

private:
	void* CreateBoxMeshObject();

	void UpdateTransform();
	void Cleanup_GameObject();

private:
	Game* m_pGame;
	D3D12Renderer* m_pRenderer;
	void* m_pMeshObj;

	XMVECTOR m_Scale;
	XMVECTOR m_Pos;
	float m_fRotY;
		
	XMMATRIX m_matScale;
	XMMATRIX m_matRot;
	XMMATRIX m_matTrans;
	XMMATRIX m_matWorld;

	bool m_bUpdateTransform;
};

