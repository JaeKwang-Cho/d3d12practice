#pragma once
class FlyCamera
{
public:
	// 카메라 초기화
	void SetCameraLookAt(const XMFLOAT3& _pos, const XMFLOAT3& _target, const XMFLOAT3& _worldUp);

	// 카메라 축 별 이동
	void Strafe(float _d);
	void Walk(float _d);
	void Ascend(float _d);

	// 카메라 회전
	void AddPitch(float _angle);
	void AddYaw(float _angle);
	void AddRoll(float _angle);

	// 카메라 속성이 바뀌었을 때, 내부 값 갱신
	void UpdateViewMatrix();
	void SetFrustum(float _fovY, float _aspectRatio, float _zn, float _zf);
protected:
private:

public:
protected:
private:
	// 카메라 로컬 좌표계 값을, 월드로 적용 시키기 위함.
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Right;
	DirectX::XMFLOAT3 m_Up;
	DirectX::XMFLOAT3 m_Look;

	// 값이 갱신 되었을 때, 켜는 플래그다.
	bool m_ViewDirty;

	// view-projection 매트릭스
	XMMATRIX m_ViewMat;
	XMMATRIX m_ProjMat;
public:
	// 위치
	XMFLOAT3 GetPosition() const {	return m_Position;	}
	void SetPosition(float _x, float _y, float _z) { m_Position = XMFLOAT3(_x, _y, _z);	}
	void SetPosition(const XMFLOAT3& _v) {m_Position = _v;}

	// 방향
	XMFLOAT3 GetRightDir() const { return m_Right; }
	XMFLOAT3 GetUpDir() const { return m_Up; }
	XMFLOAT3 GetLookDir() const { return m_Look; }

	// view, proj
	XMMATRIX GetViewMat() const { return m_ViewMat; }
	XMMATRIX GetProjMat() const { return m_ProjMat; }

	FlyCamera();
	~FlyCamera();
};

