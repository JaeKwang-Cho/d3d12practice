//***************************************************************************************
// Waves.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Performs the calculations for the wave simulation.  After the simulation has been
// updated, the client must copy the current solution into vertex buffers for rendering.
// This class only does the calculations, it does not do any drawing.
//***************************************************************************************

#pragma once

#include <vector>
#include <DirectXMath.h>

class Waves
{
public:
	Waves(int _numRows, int _numCols, float _dx, float _dt, float _speed, float _damping);
	Waves(const Waves& _rhs) = delete;
	Waves& operator=(const Waves& _rhs) = delete;
	~Waves();

public:
	void Update(float _dt);
	void Disturb(int i, int j, float _magnitude);

public:

	int RowCount() const
	{
		return m_NumRows;
	}

	int ColumnCount() const
	{
		return m_NumCols;
	}

	int VertexCount() const
	{
		return m_VertexCount;
	}

	int TriangleCount() const
	{
		return m_TriangleCount;
	}

	float Width() const
	{
		return m_NumCols * m_SpatialStep;
	}

	float Depth() const
	{
		return m_NumRows * m_SpatialStep;
	}

	// i ��° �׸��� ����Ʈ�� �ִ� �Ŀ��� ��´�.
	const DirectX::XMFLOAT3& GetSolutionPos(int i) const
	{
		return m_CurrSolution[i];
	}

	// i ��° �׸��� ����Ʈ�� �ִ� �Ŀ��� ����� ��´�.
	const DirectX::XMFLOAT3& GetSolutionNorm(int i) const
	{
		return m_Normals[i];
	}

	// i ��° �׸��� ����Ʈ�� �ִ� �Ŀ��� ź��Ʈ�� ��´�.
	const DirectX::XMFLOAT3& GetSolutionTangent(int i) const
	{
		return m_TangentX[i];
	}


private:
	int m_NumRows = 0;
	int m_NumCols = 0;

	int m_VertexCount = 0;
	int m_TriangleCount = 0;

	// �̸� ����� �� �ִ� �ùķ��̼� ��� ��
	float m_K1 = 0.f;
	float m_K2 = 0.f;
	float m_K3 = 0.f;

	float m_accumulTime = 0.f;

	float m_TimeStep = 0.f;
	float m_SpatialStep = 0.f;

	std::vector<DirectX::XMFLOAT3> m_PreSolution;
	std::vector<DirectX::XMFLOAT3> m_CurrSolution;
	std::vector<DirectX::XMFLOAT3> m_Normals;
	std::vector<DirectX::XMFLOAT3> m_TangentX;
};

