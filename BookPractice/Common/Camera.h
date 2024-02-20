//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// ���� ��ǥ��� ���ƴٴ� �� �ִ� ī�޶��̴�.
// D3D�� ���̴����� �ʱ�ȭ �ϰ�, ����� �� �ֵ��� ���������� ��Ʈ������ ������ �ִ�.
//***************************************************************************************

#pragma once
#include "d3dUtil.h"

class Camera
{
public:

	Camera();
	~Camera();

	// ī�޶� ��ġ
	DirectX::XMVECTOR GetPositionVec()const;
	DirectX::XMFLOAT3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	
	// ī�޶� �� ����
	DirectX::XMVECTOR GetRightVec()const;
	DirectX::XMFLOAT3 GetRight3f()const;
	DirectX::XMVECTOR GetUpVec()const;
	DirectX::XMFLOAT3 GetUp3f()const;
	DirectX::XMVECTOR GetLookVec()const;
	DirectX::XMFLOAT3 GetLook3f()const;

	// ī�޶� ��-��������
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// ī�޶� �ʺ�-����
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	
	// �������� �ʱ�ȭ
	void SetFrustum(float fovY, float aspect, float zn, float zf);

	// ī�޶� ���� ���� �ʱ�ȭ
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// ���������� ������ �ִ� view - projection ��Ʈ����
	DirectX::XMMATRIX GetViewMat()const;
	DirectX::XMMATRIX GetProjMat()const;

	DirectX::XMFLOAT4X4 GetView4x4f()const;
	DirectX::XMFLOAT4X4 GetProj4x4f()const;

	// ī�޶� �� �� �̵�
	void Strafe(float d);
	void Walk(float d);
	void Ascend(float d);

	// ī�޶� ȸ��
	void AddPitch(float angle);
	void AddYaw(float angle);
	void AddRoll(float angle);

	// ī�޶� �Ӽ��� �ٲ���� ��, ���� �� ����
	void UpdateViewMatrix();

private:

	// ī�޶� ���� ��ǥ�� ����, ����� ���� ��Ű�� ����.
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_Right = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_Up = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 m_Look = { 0.0f, 0.0f, 1.0f };

	// �� �������� �Ӽ� ��
	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Aspect = 0.0f;
	float m_FovY = 0.0f;
	float m_NearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	// ���� ���� �Ǿ��� ��, �Ѵ� �÷��״�.
	bool m_ViewDirty = true;

	// view-projection ��Ʈ����
	DirectX::XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();
};
