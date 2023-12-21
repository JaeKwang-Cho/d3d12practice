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

	// 정지 상태에서 타이머를 재개하는 경우
	if (m_Stopped)
	{
		// 일시 정지된 시간을 누적한다.
		m_PauseElapsedTime += (startTime - m_StopTime);

		// 이전 시간을 다시 유효하게 만든다.
		m_PrevTime = startTime;

		// 상태 갱신
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
	// 새로이 정지가 되었을 때
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

	// 현재 프레임 시간 얻기
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_CurrTime = currTime;

	// 프레임 차이 구하기
	m_DeltaTime = (m_CurrTime - m_PrevTime) * m_SecondsPerCount;

	// 음수가 되지 않게 한다.
	if (m_DeltaTime < 0.0)
	{
		m_DeltaTime = 0.0;
	}
}