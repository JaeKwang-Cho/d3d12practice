//*****************************************************************************
// Waves.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Waves.h"

#include <ppl.h> // 병렬 패턴 라이브러리
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
	
	// 몬가 수학에 쓰이는 키네틱 값을 미리 계산해 놓는다.
	// damping : 감소 폭
	float d = _damping * _dt + 2.f;
	m_K1 = (_damping * _dt - 2.f) / d;

	float e = (_speed * _speed) * (_dt * _dt) / (_dx * _dx);
	m_K2 = (4.f - 8.f * e) / d;
	m_K3 = (2.f * e) / d;

	// 그리드 사이즈 만큼 키워준다.
	m_PreSolution.resize(_numRows * _numCols);
	m_CurrSolution.resize(_numRows * _numCols);
	m_Normals.resize(_numRows * _numCols);
	m_TangentX.resize(_numRows * _numCols);

	// grid 점을 일단 메모리에 생성해 놓는다.11
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
	// 시간을 누적하고
	m_accumulTime += _dt;

	// TimpStep이 경과할때만 업데이트를 해준다.
	if (m_accumulTime >= m_TimeStep)
	{
		// 시간을 초기화 하고
		m_accumulTime = 0.f;

		// 경계 조건을 0으로 한다. 내부 점만 업데이트 한다.

		// 점 업데이트는 윈도우에서 제공하는 ppl을 사용한다.
		concurrency::parallel_for(1, m_NumRows - 1,
								  [this](int i)
								  {
									  for (int j = 1; j < m_NumCols - 1; j++)
									  {
										  // 이전 y와, 현재 y를
										  // 뭔가 키네틱 값이랑 막 곱하고 더하면
										  // 뭔가 뭔가 되는 것 같다.
										  m_PreSolution[i * m_NumCols + j].y =
											  m_K1 * m_PreSolution[i * m_NumCols + j].y + // 현재 y에는 뭔가 그냥 K1이랑 K2를 곱하고
											  m_K2 * m_CurrSolution[i * m_NumCols + j].y +
											  m_K3 * (m_CurrSolution[(i + 1) * m_NumCols + j].y + // K3는 상하좌우에 있는 값을 곱한다.
													  m_CurrSolution[(i - 1) * m_NumCols + j].y +
													  m_CurrSolution[i * m_NumCols + j + 1].y +
													  m_CurrSolution[i * m_NumCols + j - 1].y
													  );
									  }
								  });

		// Prev 에 하는 이유는, 일단 계산을 한다음에, 바꾸는 것이 메모리를 적게 쓰기 때문이다.
		std::swap(m_PreSolution, m_CurrSolution);


		// 유한 차분 법을 이용해서, normal을 근사한다.
		concurrency::parallel_for(1, m_NumRows - 1,
								  [this](int i)
								  {
									  for (int j = 1; j < m_NumCols - 1; j++)
									  {
										  // 일단 해당하는 점의 상하좌우의 y 값을 구하고
										  float left = m_CurrSolution[i * m_NumCols + j - 1].y;
										  float right = m_CurrSolution[i * m_NumCols + j + 1].y;
										  float top = m_CurrSolution[(i - 1) * m_NumCols + j].y;
										  float bottom = m_CurrSolution[(i + 1) * m_NumCols + j].y;

										  // 해당하는 점의 노멀을 이렇게. 바로 옆의 값으로 바꾼다.
										  m_Normals[i * m_NumCols + j].x = -right + left;
										  m_Normals[i * m_NumCols + j].y = 2.f * m_SpatialStep;
										  m_Normals[i * m_NumCols + j].z = bottom - top;

										  XMVECTOR iterNorm = XMLoadFloat3(&m_Normals[i * m_NumCols + j]);
										  XMVECTOR norm = XMVector3Normalize(iterNorm);
										  XMStoreFloat3(&m_Normals[i * m_NumCols + j], norm);

										  // 탄젠트도 아래와 같은 식으로 구한다..
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
	// 경계를 넘어가지 않는다.
	assert(i > 1 && i < m_NumRows - 2 && "Don't disturb boundaries");
	assert(j > 1 && j < m_NumCols - 2 && "Don't disturb boundaries");

	float halfMag = 0.5f * _magnitude;

	// (i, j)와 그 이웃들의 파동을 일으킨다.
	m_CurrSolution[i * m_NumCols + j].y += _magnitude;
	m_CurrSolution[i * m_NumCols + j + 1].y += halfMag;
	m_CurrSolution[i * m_NumCols + j - 1].y += halfMag;
	m_CurrSolution[(i + 1) * m_NumCols + j].y += halfMag;
	m_CurrSolution[(i - 1) * m_NumCols + j].y += halfMag;
}
