#pragma once
class FlyCamera
{
public:
	// ī�޶� �ʱ�ȭ
	void SetCameraLookAt(const XMFLOAT3& _pos, const XMFLOAT3& _target, const XMFLOAT3& _worldUp);

	// ī�޶� �� �� �̵�
	void Strafe(float _d);
	void Walk(float _d);
	void Ascend(float _d);

	// ī�޶� ȸ��
	void AddPitch(float _angle);
	void AddYaw(float _angle);
	void AddRoll(float _angle);

	// ī�޶� �Ӽ��� �ٲ���� ��, ���� �� ����
	void UpdateViewMatrix();
	void SetFrustum(float _fovY, float _aspectRatio, float _zn, float _zf);
protected:
private:

public:
protected:
private:
	// ī�޶� ���� ��ǥ�� ����, ����� ���� ��Ű�� ����.
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Right;
	DirectX::XMFLOAT3 m_Up;
	DirectX::XMFLOAT3 m_Look;

	// ���� ���� �Ǿ��� ��, �Ѵ� �÷��״�.
	bool m_ViewDirty;

	// view-projection ��Ʈ����
	XMMATRIX m_ViewMat;
	XMMATRIX m_ProjMat;
public:
	// ��ġ
	XMFLOAT3 GetPosition() const {	return m_Position;	}
	void SetPosition(float _x, float _y, float _z) { m_Position = XMFLOAT3(_x, _y, _z);	}
	void SetPosition(const XMFLOAT3& _v) {m_Position = _v;}

	// ����
	XMFLOAT3 GetRightDir() const { return m_Right; }
	XMFLOAT3 GetUpDir() const { return m_Up; }
	XMFLOAT3 GetLookDir() const { return m_Look; }

	// view, proj
	XMMATRIX GetViewMat() const { return m_ViewMat; }
	XMMATRIX GetProjMat() const { return m_ProjMat; }

	FlyCamera();
	~FlyCamera();
};

