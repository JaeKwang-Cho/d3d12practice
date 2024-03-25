//***************************************************************************************
// SkinnedData.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"

/// <summary>
/// keyframe : ª�� �ð��� bone�� transform�� �����Ѵ�.
/// </summary>
struct Keyframe 
{
	Keyframe();
	~Keyframe();

	float TimePos;
	DirectX::XMFLOAT3 Translation;
	DirectX::XMFLOAT3 Scale;
	DirectX::XMFLOAT4 RotationQuat;
};

/// <summary>
/// BoneAnimation : Keyframe��� �̷���� ����Ʈ.
/// Ű ������ ���� �ð��� �����Ѵ�.
/// �׻� 2���� Ű �������� �ִٰ� �����Ѵ�.
/// </summary>
struct BoneAnimation 
{
	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float _time, DirectX::XMFLOAT4X4& _Mat) const;

	std::vector<Keyframe> Keyframes;
};

/// <summary>
/// "Idle", "Walk" ���� �ִϸ��̼� Ŭ���� ���Ѵ�.
/// ��� ���� ���� ������ �ʿ��ϴ�.
/// </summary>
struct AnimationClip 
{
	float GetClipStartTime() const;
	float GetClipEndTime() const;

	void Interpolate(float _time, std::vector < DirectX::XMFLOAT4X4>& _boneTransforms) const;

	std::vector<BoneAnimation> BoneAnimations;
};

class SkinnedData
{
public:
	UINT BoneCount() const;

	float GetClipStartTime(const std::string& _clipName) const;
	float GetClipEndTime(const std::string& _clipName) const;

	void Set(
		std::vector<int>& _boneHierarchy,
		std::vector<DirectX::XMFLOAT4X4>& _boneOffsets,
		std::unordered_map<std::string, AnimationClip>& _animations
	);

	// �ݺ��ؼ� ���δٸ�, ĳ���� �س��� �͵� ���� �����̴�.
	void GetFinalTransforms(
		const std::string& _clipName, 
		float _timePos, 
		std::vector<DirectX::XMFLOAT4X4>& _finalTransforms) const;

private:
	// i��° bone�� �θ� index�� ��´�.
	std::vector<int> m_BoneHierarchy;

	std::vector<DirectX::XMFLOAT4X4> m_BonesOffsets;

	std::unordered_map<std::string, AnimationClip> m_Animations;

};

