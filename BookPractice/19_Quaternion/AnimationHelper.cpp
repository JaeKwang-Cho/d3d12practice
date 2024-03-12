//***************************************************************************************
// AnimationHelper.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "AnimationHelper.h"

using namespace DirectX;

Keyframe::Keyframe()
	: timePos(0.f), translation(0.f, 0.f, 0.f), scale(1.f, 1.f, 1.f), rotationQuat(0.f, 0.f, 0.f, 1.f)
{
}

Keyframe::~Keyframe()
{
}

float BoneAnimation::GetStartTime() const
{
	return vecKeyframe.front().timePos;
}

float BoneAnimation::GetEndTime() const
{
	return vecKeyframe.back().timePos;
}

void BoneAnimation::Interpolate(float _t, DirectX::XMFLOAT4X4& _M) const
{
	// #1 ���� �ִϸ��̼��� �������� �ʾ��� ��
	if (_t <= vecKeyframe.front().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&vecKeyframe.front().scale);
		XMVECTOR P = XMLoadFloat3(&vecKeyframe.front().translation);
		XMVECTOR Q = XMLoadFloat4(&vecKeyframe.front().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

		// M = (������) * (ȸ�� �߽��� �ǵ��� �ݴ� �̵�) * (ȸ��) * (�ٽ� ���� ��ġ�� �̵�) * (�̵�);
		XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// #2 �ִϸ��̼� �� �κ��� ��
	else if (_t >= vecKeyframe.back().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&vecKeyframe.back().scale);
		XMVECTOR P = XMLoadFloat3(&vecKeyframe.back().translation);
		XMVECTOR Q = XMLoadFloat4(&vecKeyframe.back().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

		// M = (������) * (ȸ�� �߽��� �ǵ��� �ݴ� �̵�) * (ȸ��) * (�ٽ� ���� ��ġ�� �̵�) * (�̵�);
		XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else // #3 �ִϸ��̼� ���� ���� �� 
	{	// �ణ �� ��ȿ�� �������� �ϴ��� �̷��� �Ѵ�.
		for (UINT i = 0; i < vecKeyframe.size() - 1; i++)
		{
			if (_t >= vecKeyframe[i].timePos && _t <= vecKeyframe[i + 1].timePos)
			{
				float lerpPercent = (_t - vecKeyframe[i].timePos) / (vecKeyframe[i + 1].timePos - vecKeyframe[i].timePos);

				XMVECTOR s0 = XMLoadFloat3(&vecKeyframe[i].scale);
				XMVECTOR s1 = XMLoadFloat3(&vecKeyframe[i + 1].scale);

				XMVECTOR p0 = XMLoadFloat3(&vecKeyframe[i].translation);
				XMVECTOR p1 = XMLoadFloat3(&vecKeyframe[i + 1].translation);

				XMVECTOR q0 = XMLoadFloat4(&vecKeyframe[i].rotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&vecKeyframe[i + 1].rotationQuat);

				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				// ���� ���� ���� (spherical linear interpolation)
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
				// M = (������) * (ȸ�� �߽��� �ǵ��� �ݴ� �̵�) * (ȸ��) * (�ٽ� ���� ��ġ�� �̵�) * (�̵�);
				XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}
