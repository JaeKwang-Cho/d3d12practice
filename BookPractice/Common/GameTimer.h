#pragma once

class GameTimer
{
public:
	GameTimer();

	float GetTotalTime() const
	{
		// �Ͻ� ���� ������ ��
		if (m_Stopped)
		{
			// �⺻�� m_StopTime�� ��ȯ�Ѵ�.
			// �Ͻ����� �� ���¿��� ������ �ð��� ���Խ�Ű�� �ȵȴ�.
			return (float)(
				((m_StopTime - m_PauseElapsedTime) - m_BaseTime) * m_SecondsPerCount
				);
		}
		// �׳�
		else
		{
			// ������ ������ �� �����س��Ҵ� m_PauseElapsedTime �ð��� ����.
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
	void Reset(); // ���� ���� ��
	void Start(); // Ÿ�̸� ������ �� 
	void Stop(); // Ÿ�̸� ���� ��
	void Tick(); // �� ������

private:
	double m_SecondsPerCount; // 1�ʿ� ������ ���� ī���� ����
	double m_DeltaTime;

	__int64 m_BaseTime; // Ÿ�̸� �ʱ�ȭ �ð�
	__int64 m_PauseElapsedTime; // �� ���� �ð�
	__int64 m_StopTime; // ���� �ð�
	__int64 m_PrevTime; // ���� ������ �ð�
	__int64 m_CurrTime; // ���� ������ �ð�

	bool	m_Stopped;
};

