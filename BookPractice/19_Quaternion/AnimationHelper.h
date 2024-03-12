//***************************************************************************************
// AnimationHelper.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Keyframe, BoneAnimation 구조체(클래스)
//***************************************************************************************
#pragma once

#include "../Common/d3dUtil.h"

///<summmary>
/// keyframe : 짧은 시간에 bone의 transform를 정의한다.
///</summary>
struct Keyframe {
	Keyframe();
	~Keyframe();

	float timePos;
	DirectX::XMFLOAT3 translation;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT4 rotationQuat;
};

///<summary>
/// BoneAnimation : Keyframe들로 이루어진 리스트.
/// 키 프레임 사이 시간을 보간한다.
/// 항상 2개의 키 프레임이 있다고 가정한다.
///</summary>

struct BoneAnimation {
	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float _t, DirectX::XMFLOAT4X4& _M) const;

	std::vector<Keyframe> vecKeyframe;
};
