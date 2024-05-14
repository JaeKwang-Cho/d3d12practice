//***************************************************************************************
// SkinnedData.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "SkinnedData.h"

using namespace DirectX;

Keyframe::Keyframe()
	:TimePos(0.f), Translation(0.f, 0.f, 0.f), Scale(1.f, 1.f, 1.f), RotationQuat(0.f, 0.f, 0.f, 1.f)
{

}

Keyframe::~Keyframe()
{
}

float BoneAnimation::GetStartTime() const
{
	if (Keyframes.size() <= 0) {
		return 0.f;
	}
	else {
		return Keyframes.front().TimePos;
	}
}

float BoneAnimation::GetEndTime() const
{
	if (Keyframes.size() <= 0) {
		return 0.f;
	}
	else {
		return Keyframes.back().TimePos;
	}
	
}

void BoneAnimation::Interpolate(float _time, DirectX::XMFLOAT4X4& _Mat) const
{
	// #0 키 프레임이 존재하지 않을 때
	if (Keyframes.size() <= 0) {
		XMFLOAT3 ones = XMFLOAT3(1.f, 1.f, 1.f);
		XMFLOAT3 zeros = XMFLOAT3(0.f, 0.f, 0.f);
		XMVECTOR S = XMLoadFloat3(&ones);
		XMVECTOR P = XMLoadFloat3(&zeros);
		XMVECTOR Q = XMQuaternionIdentity();

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		// M = (스케일) * (회전 중심이 되도록 반대 이동) * (회전) * (다시 원래 위치로 이동) * (이동)
		XMStoreFloat4x4(&_Mat, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// #1 아직 애니메이션이 시작하지 않았을 때
	else if (_time <= Keyframes.front().TimePos)
	{
		XMVECTOR S = XMLoadFloat3(&Keyframes.front().Scale);
		XMVECTOR P = XMLoadFloat3(&Keyframes.front().Translation);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.front().RotationQuat);

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		// M = (스케일) * (회전 중심이 되도록 반대 이동) * (회전) * (다시 원래 위치로 이동) * (이동)
		XMStoreFloat4x4(&_Mat, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// #2 애니메이션 끝 부분일 때
	else if (_time >= Keyframes.back().TimePos)
	{
		XMVECTOR S = XMLoadFloat3(&Keyframes.back().Scale);
		XMVECTOR P = XMLoadFloat3(&Keyframes.back().Translation);
		XMVECTOR Q = XMLoadFloat4(&Keyframes.back().RotationQuat);

		XMVECTOR zero = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		// M = (스케일) * (회전 중심이 되도록 반대 이동) * (회전) * (다시 원래 위치로 이동) * (이동)
		XMStoreFloat4x4(&_Mat, XMMatrixAffineTransformation(S, zero, Q, P));
	}
	// #3 애니메이션 진행 중일 때 
	else
	{
		for (UINT i = 0; i < Keyframes.size() - 1; i++)
		{
			if (_time >= Keyframes[i].TimePos && _time <= Keyframes[i + 1].TimePos)
			{
				float lerpPercent = (_time - Keyframes[i].TimePos) / (Keyframes[i + 1].TimePos - Keyframes[i].TimePos);

				XMVECTOR s0 = XMLoadFloat3(&Keyframes[i].Scale);
				XMVECTOR s1 = XMLoadFloat3(&Keyframes[i + 1].Scale);

				XMVECTOR p0 = XMLoadFloat3(&Keyframes[i].Translation);
				XMVECTOR p1 = XMLoadFloat3(&Keyframes[i + 1].Translation);

				XMVECTOR q0 = XMLoadFloat4(&Keyframes[i].RotationQuat);
				XMVECTOR q1 = XMLoadFloat4(&Keyframes[i + 1].RotationQuat);

				XMVECTOR S = XMVectorLerp(s0, s1, lerpPercent);
				XMVECTOR P = XMVectorLerp(p0, p1, lerpPercent);
				XMVECTOR Q = XMQuaternionSlerp(q0, q1, lerpPercent);

				XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
				XMStoreFloat4x4(&_Mat, XMMatrixAffineTransformation(S, zero, Q, P));

				break;
			}
		}
	}
}

float AnimationClip::GetClipStartTime() const
{
	// 모든 bones에서 가장 먼저 시작하는 친구를 찾는다.
	float t = MathHelper::Infinity;
	for (UINT i = 0; i < BoneAnimations.size(); i++)
	{
		t = MathHelper::Min(t, BoneAnimations[i].GetStartTime());
	}
	return t;
}

float AnimationClip::GetClipEndTime() const
{
	// 모든 bones에서 가장 늦게 끝나는 친구를 찾는다.
	float t = 0.f;
	for (UINT i = 0; i < BoneAnimations.size(); i++)
	{
		t = MathHelper::Max(t, BoneAnimations[i].GetEndTime());
	}
	return t;
}

void AnimationClip::Interpolate(float _time, std::vector<DirectX::XMFLOAT4X4>& _boneTransforms) const
{
	for (UINT i = 0; i < BoneAnimations.size(); i++)
	{
		BoneAnimations[i].Interpolate(_time, _boneTransforms[i]);
	}
}

float SkinnedData::GetClipStartTime(const std::string& _clipName) const
{
	std::unordered_map<std::string, AnimationClip>::const_iterator clip = m_Animations.find(_clipName);
	return clip->second.GetClipStartTime();
}

float SkinnedData::GetClipEndTime(const std::string& _clipName) const
{
	std::unordered_map<std::string, AnimationClip>::const_iterator clip = m_Animations.find(_clipName);
	return clip->second.GetClipEndTime();
}

UINT SkinnedData::BoneCount() const
{
	return (UINT)m_BoneHierarchy.size();
}

void SkinnedData::Set(std::vector<int>& _boneHierarchy, std::vector<DirectX::XMFLOAT4X4>& _boneOffsets, std::unordered_map<std::string, AnimationClip>& _animations)
{
	m_BoneHierarchy = _boneHierarchy;
	m_BonesOffsets = _boneOffsets;
	m_Animations = _animations;
}

void SkinnedData::GetFinalTransforms(const std::string& _clipName, float _timePos, std::vector<DirectX::XMFLOAT4X4>& _finalTransforms) const
{
	UINT numOfBones = (UINT)m_BonesOffsets.size();

	std::vector<XMFLOAT4X4> toParentTransforms(numOfBones);

	// 주어진 시간에 따라 현재 애니메이션 clip의 모든 bones을 보간한다.
	auto clip = m_Animations.find(_clipName);
	clip->second.Interpolate(_timePos, toParentTransforms);

	// hierarchy를 traverse 하면서 모든 bone을 root space로 transform 시키는 행렬을 저장한다.
	std::vector<XMFLOAT4X4> toRootTransforms(numOfBones);

	// root bone은 0번이다. 따로 부모가 없으니 자기 자신을 부모로 한다. 
	// (얘는 그냥 local 좌표계에서 계산하는것과 같은샘)
	toRootTransforms[0] = toParentTransforms[0];

	// 자식 bone을 돌아다니면서 Root 좌표계로 바꾸는 행렬을 계산한다.
	for (UINT i = 1; i < numOfBones; i++)
	{
		XMMATRIX toParent = XMLoadFloat4x4(&toParentTransforms[i]);

		int parentIndex = m_BoneHierarchy[i];
		XMMATRIX parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);

		XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);

		XMStoreFloat4x4(&toRootTransforms[i], toRoot);
	}

	// final transform을 얻기위해 offset transform으로 미리 곱해놓는다.
	for (UINT i = 0; i < numOfBones; i++)
	{
		XMMATRIX offset = XMLoadFloat4x4(&m_BonesOffsets[i]);
		XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[i]);
		XMMATRIX finalTransform = XMMatrixMultiply(offset, toRoot);
		XMStoreFloat4x4(&_finalTransforms[i], XMMatrixTranspose(finalTransform));
	}

}
