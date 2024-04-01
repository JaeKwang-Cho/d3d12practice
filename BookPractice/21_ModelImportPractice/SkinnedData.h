//***************************************************************************************
// SkinnedData.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"

/// <summary>
/// keyframe : 짧은 시간에 bone의 transform를 정의한다.
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
/// BoneAnimation : Keyframe들로 이루어진 리스트.
/// 키 프레임 사이 시간을 보간한다.
/// 항상 2개의 키 프레임이 있다고 가정한다.
/// </summary>
struct BoneAnimation 
{
	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float _time, DirectX::XMFLOAT4X4& _Mat) const;

	std::vector<Keyframe> Keyframes;
};

/// <summary>
/// "Idle", "Walk" 같은 애니메이션 클립을 말한다.
/// 모든 본에 대한 정보가 필요하다.
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

	// 반복해서 쓰인다면, 캐싱을 해놓는 것도 좋은 생각이다.
	void GetFinalTransforms(
		const std::string& _clipName, 
		float _timePos, 
		std::vector<DirectX::XMFLOAT4X4>& _finalTransforms) const;

private:
	// i번째 bone의 부모 index를 얻는다.
	std::vector<int> m_BoneHierarchy;

	std::vector<DirectX::XMFLOAT4X4> m_BonesOffsets;

	std::unordered_map<std::string, AnimationClip> m_Animations;

};

