//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	SetFrustum(0.25f*MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
}

Camera::~Camera()
{
}

XMVECTOR Camera::GetPositionVec()const
{
	return XMLoadFloat3(&m_Position);
}

XMFLOAT3 Camera::GetPosition3f()const
{
	return m_Position;
}

void Camera::SetPosition(float x, float y, float z)
{
	m_Position = XMFLOAT3(x, y, z);
	m_ViewDirty = true;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	m_Position = v;
	m_ViewDirty = true;
}

XMVECTOR Camera::GetRightVec()const
{
	return XMLoadFloat3(&m_Right);
}

XMFLOAT3 Camera::GetRight3f()const
{
	return m_Right;
}

XMVECTOR Camera::GetUpVec()const
{
	return XMLoadFloat3(&m_Up);
}

XMFLOAT3 Camera::GetUp3f()const
{
	return m_Up;
}

XMVECTOR Camera::GetLookVec()const
{
	return XMLoadFloat3(&m_Look);
}

XMFLOAT3 Camera::GetLook3f()const
{
	return m_Look;
}

float Camera::GetNearZ()const
{
	return m_NearZ;
}

float Camera::GetFarZ()const
{
	return m_FarZ;
}

float Camera::GetAspect()const
{
	return m_Aspect;
}

float Camera::GetFovY()const
{
	return m_FovY;
}

float Camera::GetFovX()const
{
	float halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f*atan(halfWidth / m_NearZ);
}

float Camera::GetNearWindowWidth()const
{
	return m_Aspect * m_NearWindowHeight;
}

float Camera::GetNearWindowHeight()const
{
	return m_NearWindowHeight;
}

float Camera::GetFarWindowWidth()const
{
	return m_Aspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight()const
{
	return mFarWindowHeight;
}

void Camera::SetFrustum(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	m_FovY = fovY;
	m_Aspect = aspect;
	m_NearZ = zn;
	m_FarZ = zf;

	m_NearWindowHeight = 2.0f * m_NearZ * tanf( 0.5f*m_FovY );
	mFarWindowHeight  = 2.0f * m_FarZ * tanf( 0.5f*m_FovY );

	XMMATRIX P = XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearZ, m_FarZ);
	XMStoreFloat4x4(&m_ProjMat, P);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_Position, pos);
	XMStoreFloat3(&m_Look, L);
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);

	m_ViewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	m_ViewDirty = true;
}

XMMATRIX Camera::GetViewMat()const
{
	assert(!m_ViewDirty);
	return XMLoadFloat4x4(&m_ViewMat);
}

XMMATRIX Camera::GetProjMat()const
{
	return XMLoadFloat4x4(&m_ProjMat);
}


XMFLOAT4X4 Camera::GetView4x4f()const
{
	assert(!m_ViewDirty);
	return m_ViewMat;
}

XMFLOAT4X4 Camera::GetProj4x4f()const
{
	return m_ProjMat;
}

void Camera::Strafe(float d)
{
	// m_Position += d*m_Right
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&m_Right);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, r, p));

	m_ViewDirty = true;
}

void Camera::Walk(float d)
{
	// m_Position += d*m_Look
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&m_Look);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));

	m_ViewDirty = true;
}

void Camera::Ascend(float d)
{
	// m_Position += d*m_Look
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&m_Up);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));

	m_ViewDirty = true;
}

void Camera::AddPitch(float angle)
{
	// Rotate up and look vector about the right vector.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), angle);

	XMStoreFloat3(&m_Up,   XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	m_ViewDirty = true;
}

void Camera::AddYaw(float angle)
{
	// Rotate the basis vectors about the world y-axis.

	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&m_Right,   XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	m_ViewDirty = true;
}

void Camera::UpdateViewMatrix()
{
	if(m_ViewDirty)
	{
		XMVECTOR R = XMLoadFloat3(&m_Right);
		XMVECTOR U = XMLoadFloat3(&m_Up);
		XMVECTOR L = XMLoadFloat3(&m_Look);
		XMVECTOR P = XMLoadFloat3(&m_Position);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&m_Right, R);
		XMStoreFloat3(&m_Up, U);
		XMStoreFloat3(&m_Look, L);

		m_ViewMat(0, 0) = m_Right.x;
		m_ViewMat(1, 0) = m_Right.y;
		m_ViewMat(2, 0) = m_Right.z;
		m_ViewMat(3, 0) = x;

		m_ViewMat(0, 1) = m_Up.x;
		m_ViewMat(1, 1) = m_Up.y;
		m_ViewMat(2, 1) = m_Up.z;
		m_ViewMat(3, 1) = y;

		m_ViewMat(0, 2) = m_Look.x;
		m_ViewMat(1, 2) = m_Look.y;
		m_ViewMat(2, 2) = m_Look.z;
		m_ViewMat(3, 2) = z;

		m_ViewMat(0, 3) = 0.0f;
		m_ViewMat(1, 3) = 0.0f;
		m_ViewMat(2, 3) = 0.0f;
		m_ViewMat(3, 3) = 1.0f;

		m_ViewDirty = false;
	}
}


