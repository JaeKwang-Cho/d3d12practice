//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer()
	: m_SecondsPerCount(0.0),
	m_DeltaTime(0.0),
	m_BaseTime(0),
	m_PauseElapsedTime(0),
	m_StopTime(0),
	m_PrevTime(0),
	m_CurrTime(0),
	m_Stopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_SecondsPerCount = 1.0 / (double)countsPerSec;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_BaseTime = currTime;
	m_PrevTime = currTime;
	m_StopTime = 0;
	m_Stopped = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	// ���� ���¿��� Ÿ�̸Ӹ� �簳�ϴ� ���
	if (m_Stopped)
	{
		// �Ͻ� ������ �ð��� �����Ѵ�.
		m_PauseElapsedTime += (startTime - m_StopTime);

		// ���� �ð��� �ٽ� ��ȿ�ϰ� �����.
		m_PrevTime = startTime;

		// ���� ����
		m_StopTime = 0;
		m_Stopped = false;
	}
}

void GameTimer::Stop()
{
	if (m_Stopped)
	{
		return;
	}
	// ������ ������ �Ǿ��� ��
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_StopTime = currTime;
	m_Stopped = true;
}

void GameTimer::Tick()
{
	if (m_Stopped)
	{
		m_DeltaTime = 0.0;
		return;
	}

	// ���� ������ �ð� ���
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_CurrTime = currTime;

	// ������ ���� ���ϱ�
	m_DeltaTime = (m_CurrTime - m_PrevTime) * m_SecondsPerCount;

	// ������ ���� �ʰ� �Ѵ�.
	if (m_DeltaTime < 0.0)
	{
		m_DeltaTime = 0.0;
	}
}