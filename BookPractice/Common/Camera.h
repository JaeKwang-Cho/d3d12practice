//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// 로컬 좌표계로 날아다닐 수 있는 카메라이다.
// D3D나 쉐이더에서 초기화 하고, 사용할 수 있도록 내부적으로 매트릭스를 가지고 있다.
//***************************************************************************************

#pragma once
#include "d3dUtil.h"

class Camera
{
public:

	Camera();
	~Camera();

	// 카메라 위치
	DirectX::XMVECTOR GetPositionVec()const;
	DirectX::XMFLOAT3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	
	// 카메라 축 벡터
	DirectX::XMVECTOR GetRightVec()const;
	DirectX::XMFLOAT3 GetRight3f()const;
	DirectX::XMVECTOR GetUpVec()const;
	DirectX::XMFLOAT3 GetUp3f()const;
	DirectX::XMVECTOR GetLookVec()const;
	DirectX::XMFLOAT3 GetLook3f()const;

	// 카메라 뷰-프러스텀
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// 카메라 너비-깊이
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	
	// 프러스텀 초기화
	void SetFrustum(float fovY, float aspect, float zn, float zf);

	// 카메라가 보는 방향 초기화
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// 내부적으로 가지고 있는 view - projection 매트릭스
	DirectX::XMMATRIX GetViewMat()const;
	DirectX::XMMATRIX GetProjMat()const;

	DirectX::XMFLOAT4X4 GetView4x4f()const;
	DirectX::XMFLOAT4X4 GetProj4x4f()const;

	// 카메라 축 별 이동
	void Strafe(float d);
	void Walk(float d);
	void Ascend(float d);

	// 카메라 회전
	void AddPitch(float angle);
	void AddYaw(float angle);
	void AddRoll(float angle);

	// 카메라 속성이 바뀌었을 때, 내부 값 갱신
	void UpdateViewMatrix();

private:

	// 카메라 로컬 좌표계 값을, 월드로 적용 시키기 위함.
	DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_Right = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_Up = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 m_Look = { 0.0f, 0.0f, 1.0f };

	// 뷰 프러스텀 속성 값
	float m_NearZ = 0.0f;
	float m_FarZ = 0.0f;
	float m_Aspect = 0.0f;
	float m_FovY = 0.0f;
	float m_NearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	// 값이 갱신 되었을 때, 켜는 플래그다.
	bool m_ViewDirty = true;

	// view-projection 매트릭스
	DirectX::XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();
};
