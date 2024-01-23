//*****************************************************************************
// Waves.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Waves.h"

#include <ppl.h> // ���� ���� ���̺귯��
#include <algorithm>
#include <vector>
#include <cassert>

#include <string>

using namespace DirectX;

Waves::Waves(int _numRows, int _numCols, float _dx, float _dt, float _speed, float _damping)
{
	m_NumRows = _numRows;
	m_NumCols = _numCols;

	m_VertexCount = _numRows * _numCols;
	m_TriangleCount = (_numRows - 1) * (_numCols - 1) * 2;

	m_SpatialStep = _dx;
	m_TimeStep = _dt;
	
	// �� ���п� ���̴� Ű��ƽ ���� �̸� ����� ���´�.
	// damping : ���� ��
	float d = _damping * _dt + 2.f;
	m_K1 = (_damping * _dt - 2.f) / d;

	float e = (_speed * _speed) * (_dt * _dt) / (_dx * _dx);
	m_K2 = (4.f - 8.f * e) / d;
	m_K3 = (2.f * e) / d;

	// �׸��� ������ ��ŭ Ű���ش�.
	m_PreSolution.resize(_numRows * _numCols);
	m_CurrSolution.resize(_numRows * _numCols);
	m_Normals.resize(_numRows * _numCols);
	m_TangentX.resize(_numRows * _numCols);

	// grid ���� �ϴ� �޸𸮿� ������ ���´�.11
	float halfWidth = (_numRows - 1) * _dx * 0.5f;
	float halfDepth = (_numCols - 1) * _dx * 0.5f;
	
	for (int i = 0; i < _numRows; i++)
	{
		float z = halfDepth - i * _dx;
		for (int j = 0; j < _numCols; j++)
		{
			float x = - halfWidth + j * _dx;

			m_PreSolution[i * _numCols + j] = XMFLOAT3(x, 0.f, z);
			m_CurrSolution[i * _numCols + j] = XMFLOAT3(x, 0.f, z);
			m_Normals[i * _numCols + j] = XMFLOAT3(0.f, 1.f, 0.f);
			m_TangentX[i * _numCols + j] = XMFLOAT3(1.f, 0.f, 0.f);
		}
	}
	int i = 0;
}

Waves::~Waves()
{
}

void Waves::Update(float _dt)
{
	// �ð��� �����ϰ�
	m_accumulTime += _dt;

	// TimpStep�� ����Ҷ��� ������Ʈ�� ���ش�.
	if (m_accumulTime >= m_TimeStep)
	{
		// �ð��� �ʱ�ȭ �ϰ�
		m_accumulTime = 0.f;

		// ��� ������ 0���� �Ѵ�. ���� ���� ������Ʈ �Ѵ�.

		// �� ������Ʈ�� �����쿡�� �����ϴ� ppl�� ����Ѵ�.
		concurrency::parallel_for(1, m_NumRows - 1,
								  [this](int i)
								  {
									  for (int j = 1; j < m_NumCols - 1; j++)
									  {
										  // ���� y��, ���� y��
										  // ���� Ű��ƽ ���̶� �� ���ϰ� ���ϸ�
										  // ���� ���� �Ǵ� �� ����.
										  m_PreSolution[i * m_NumCols + j].y =
											  m_K1 * m_PreSolution[i * m_NumCols + j].y + // ���� y���� ���� �׳� K1�̶� K2�� ���ϰ�
											  m_K2 * m_CurrSolution[i * m_NumCols + j].y +
											  m_K3 * (m_CurrSolution[(i + 1) * m_NumCols + j].y + // K3�� �����¿쿡 �ִ� ���� ���Ѵ�.
													  m_CurrSolution[(i - 1) * m_NumCols + j].y +
													  m_CurrSolution[i * m_NumCols + j + 1].y +
													  m_CurrSolution[i * m_NumCols + j - 1].y
													  );
									  }
								  });

		// Prev �� �ϴ� ������, �ϴ� ����� �Ѵ�����, �ٲٴ� ���� �޸𸮸� ���� ���� �����̴�.
		std::swap(m_PreSolution, m_CurrSolution);


		// ���� ���� ���� �̿��ؼ�, normal�� �ٻ��Ѵ�.
		concurrency::parallel_for(1, m_NumRows - 1,
								  [this](int i)
								  {
									  for (int j = 1; j < m_NumCols - 1; j++)
									  {
										  // �ϴ� �ش��ϴ� ���� �����¿��� y ���� ���ϰ�
										  float left = m_CurrSolution[i * m_NumCols + j - 1].y;
										  float right = m_CurrSolution[i * m_NumCols + j + 1].y;
										  float top = m_CurrSolution[(i - 1) * m_NumCols + j].y;
										  float bottom = m_CurrSolution[(i + 1) * m_NumCols + j].y;

										  // �ش��ϴ� ���� ����� �̷���. �ٷ� ���� ������ �ٲ۴�.
										  m_Normals[i * m_NumCols + j].x = -right + left;
										  m_Normals[i * m_NumCols + j].y = 2.f * m_SpatialStep;
										  m_Normals[i * m_NumCols + j].z = bottom - top;

										  XMVECTOR iterNorm = XMLoadFloat3(&m_Normals[i * m_NumCols + j]);
										  XMVECTOR norm = XMVector3Normalize(iterNorm);
										  XMStoreFloat3(&m_Normals[i * m_NumCols + j], norm);

										  // ź��Ʈ�� �Ʒ��� ���� ������ ���Ѵ�..
										  m_TangentX[i * m_NumCols + j] = XMFLOAT3(2.f * m_SpatialStep, right - left, 0.f);
										  XMVECTOR iterTanX = XMLoadFloat3(&m_TangentX[i * m_NumCols + j]);
										  XMVECTOR TanX = XMVector3Normalize(iterTanX);
										  XMStoreFloat3(&m_TangentX[i * m_NumCols + j], TanX);
									  }
								  });

	}
}

void Waves::Disturb(int i, int j, float _magnitude)
{
	// ��踦 �Ѿ�� �ʴ´�.
	assert(i > 1 && i < m_NumRows - 2 && "Don't disturb boundaries");
	assert(j > 1 && j < m_NumCols - 2 && "Don't disturb boundaries");

	float halfMag = 0.5f * _magnitude;

	// (i, j)�� �� �̿����� �ĵ��� ����Ų��.
	m_CurrSolution[i * m_NumCols + j].y += _magnitude;
	m_CurrSolution[i * m_NumCols + j + 1].y += halfMag;
	m_CurrSolution[i * m_NumCols + j - 1].y += halfMag;
	m_CurrSolution[(i + 1) * m_NumCols + j].y += halfMag;
	m_CurrSolution[(i - 1) * m_NumCols + j].y += halfMag;
}
