#include "pch.h"
#include "FlyCamera.h"

void FlyCamera::SetCameraLookAt(const XMFLOAT3& _pos, const XMFLOAT3& _target, const XMFLOAT3& _worldUp)
{
	XMVECTOR pos = XMLoadFloat3(&_pos);
	XMVECTOR target = XMLoadFloat3(&_target);
	XMVECTOR worldUp = XMLoadFloat3(&_worldUp);

	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_Position, pos);
	XMStoreFloat3(&m_Look, L);
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);

	m_ViewDirty = true;
}

void FlyCamera::Strafe(float _d)
{
	// m_Position += d*m_Right
	XMVECTOR s = XMVectorReplicate(_d);
	XMVECTOR r = XMLoadFloat3(&m_Right);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, r, p));

	m_ViewDirty = true;
}

void FlyCamera::Walk(float _d)
{
	// m_Position += d*m_Look
	XMVECTOR s = XMVectorReplicate(_d);
	XMVECTOR l = XMLoadFloat3(&m_Look);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));

	m_ViewDirty = true;
}

void FlyCamera::Ascend(float _d)
{
	// m_Position += d*m_Up
	XMVECTOR s = XMVectorReplicate(_d);
	XMVECTOR l = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));

	m_ViewDirty = true;
}

void FlyCamera::AddPitch(float _angle)
{
	// ���� ������ ���͸� �������� ȸ����Ų��.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), _angle);

	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	m_ViewDirty = true;
}

void FlyCamera::AddYaw(float _angle)
{
	//���� y ���͸� �������� ȸ�� ��Ų��.
	XMMATRIX R = XMMatrixRotationY(_angle);

	XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	m_ViewDirty = true;
}

void FlyCamera::AddRoll(float _angle)
{
	// ���� ���� ���͸� �������� ȸ�� ��Ų��..

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Look), _angle);

	XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));

	m_ViewDirty = true;
}

void FlyCamera::UpdateViewMatrix()
{
	if (m_ViewDirty)
	{
		XMVECTOR R = XMLoadFloat3(&m_Right);
		XMVECTOR U = XMLoadFloat3(&m_Up);
		XMVECTOR L = XMLoadFloat3(&m_Look);
		XMVECTOR P = XMLoadFloat3(&m_Position);

		// ���� ī�޶� ���� ��Ÿ���°����� ����, ������ �ǵ��� ������Ų��.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// up ���Ϳ� front ���ͷ� right ���͸� ���Ѵ�.
		R = XMVector3Cross(U, L);

		// view ��Ʈ������ ���Ĵ�� ä���.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&m_Right, R);
		XMStoreFloat3(&m_Up, U);
		XMStoreFloat3(&m_Look, L);

		XMFLOAT4X4 viewMat{};

		viewMat(0, 0) = m_Right.x;
		viewMat(1, 0) = m_Right.y;
		viewMat(2, 0) = m_Right.z;
		viewMat(3, 0) = x;
		
		viewMat(0, 1) = m_Up.x;
		viewMat(1, 1) = m_Up.y;
		viewMat(2, 1) = m_Up.z;
		viewMat(3, 1) = y;
		
		viewMat(0, 2) = m_Look.x;
		viewMat(1, 2) = m_Look.y;
		viewMat(2, 2) = m_Look.z;
		viewMat(3, 2) = z;
		
		viewMat(0, 3) = 0.0f;
		viewMat(1, 3) = 0.0f;
		viewMat(2, 3) = 0.0f;
		viewMat(3, 3) = 1.0f;

		m_ViewMat = XMLoadFloat4x4(&viewMat);

		m_ViewDirty = false;
	}
}

void FlyCamera::SetFrustum(float _fovY, float _aspectRatio, float _zn, float _zf)
{
	m_ProjMat = XMMatrixPerspectiveFovLH(_fovY, _aspectRatio, _zn, _zf);
}

FlyCamera::FlyCamera()
	:m_Position{0.f, 0.f, 0.f}, m_Right{1.f, 0.f, 0.f},
	m_Up{0.f, 1.f, 0.f}, m_Look{0.f, 0.f, 1.f},
	m_ViewDirty(true), m_ViewMat{}, m_ProjMat{}
{
}

FlyCamera::~FlyCamera()
{
}