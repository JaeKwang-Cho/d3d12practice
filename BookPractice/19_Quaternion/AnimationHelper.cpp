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
	// #1 아직 애니메이션이 시작하지 않았을 때
	if (_t <= vecKeyframe.front().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&vecKeyframe.front().scale);
		XMVECTOR P = XMLoadFloat3(&vecKeyframe.front().translation);
		XMVECTOR Q = XMLoadFloat4(&vecKeyframe.front().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

		// M = (스케일) * (회전 중심이 되도록 반대 이동) * (회전) * (다시 원래 위치로 이동) * (이동);
		XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// #2 애니메이션 끝 부분일 때
	else if (_t >= vecKeyframe.back().timePos)
	{
		XMVECTOR S = XMLoadFloat3(&vecKeyframe.back().scale);
		XMVECTOR P = XMLoadFloat3(&vecKeyframe.back().translation);
		XMVECTOR Q = XMLoadFloat4(&vecKeyframe.back().rotationQuat);

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);

		// M = (스케일) * (회전 중심이 되도록 반대 이동) * (회전) * (다시 원래 위치로 이동) * (이동);
		XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	else // #3 애니메이션 진행 중일 때 
	{	// 약간 좀 비효율 적이지만 일단은 이렇게 한다.
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
				// 구면 선형 보간 (spherical linear interpolation)
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
				// M = (스케일) * (회전 중심이 되도록 반대 이동) * (회전) * (다시 원래 위치로 이동) * (이동);
				XMStoreFloat4x4(&_M, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}
