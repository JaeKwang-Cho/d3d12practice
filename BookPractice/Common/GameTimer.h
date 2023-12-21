#pragma once

class GameTimer
{
public:
	GameTimer();

	float GetTotalTime() const
	{
		// 일시 정지 상태일 때
		if (m_Stopped)
		{
			// 기본은 m_StopTime을 반환한다.
			// 일시정지 된 상태에서 지나간 시간은 포함시키면 안된다.
			return (float)(
				((m_StopTime - m_PauseElapsedTime) - m_BaseTime) * m_SecondsPerCount
				);
		}
		// 그냥
		else
		{
			// 예전에 멈췄을 때 누적해놓았던 m_PauseElapsedTime 시간을 뺀다.
			return (float)(
				((m_CurrTime - m_PauseElapsedTime) - m_BaseTime) * m_SecondsPerCount
				);
		}
	}
	float GetDeltaTime() const
	{
		return (float)m_DeltaTime;
	}

public:
	void Reset(); // 루프 시작 전
	void Start(); // 타이머 시작할 때 
	void Stop(); // 타이머 멈출 때
	void Tick(); // 매 프레임

private:
	double m_SecondsPerCount; // 1초에 찍히는 성능 카운터 개수
	double m_DeltaTime;

	__int64 m_BaseTime; // 타이머 초기화 시각
	__int64 m_PauseElapsedTime; // 총 멈춘 시간
	__int64 m_StopTime; // 멈춘 시각
	__int64 m_PrevTime; // 이전 프레임 시각
	__int64 m_CurrTime; // 현재 프레임 시각

	bool	m_Stopped;
};

