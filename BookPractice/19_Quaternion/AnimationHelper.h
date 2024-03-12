//***************************************************************************************
// AnimationHelper.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Keyframe, BoneAnimation ����ü(Ŭ����)
//***************************************************************************************
#pragma once

#include "../Common/d3dUtil.h"

///<summmary>
/// keyframe : ª�� �ð��� bone�� transform�� �����Ѵ�.
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
/// BoneAnimation : Keyframe��� �̷���� ����Ʈ.
/// Ű ������ ���� �ð��� �����Ѵ�.
/// �׻� 2���� Ű �������� �ִٰ� �����Ѵ�.
///</summary>

struct BoneAnimation {
	float GetStartTime() const;
	float GetEndTime() const;

	void Interpolate(float _t, DirectX::XMFLOAT4X4& _M) const;

	std::vector<Keyframe> vecKeyframe;
};
