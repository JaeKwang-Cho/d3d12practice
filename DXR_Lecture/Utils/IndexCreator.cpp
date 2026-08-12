// IndexCreator.cpp from "megayuchi"

#include "pch.h"
#include "IndexCreator.h"
#ifdef __INTELLISENSE__
#include "../DXRPractice/pch.h"
#endif

bool IndexCreator::Initialize(ULONG _ulNum)
{
	m_pulIndexTable = new ULONG[_ulNum];
	if (!m_pulIndexTable) {
		__debugbreak();
		false;
	}
	memset(m_pulIndexTable, 0, sizeof(ULONG) * _ulNum);
	m_ulMaxNum = _ulNum;

	for (ULONG i = 0; i < m_ulMaxNum; i++) {
		m_pulIndexTable[i] = i;
	}

	return true;
}

ULONG IndexCreator::Alloc()
{
	// 1. Alloc에서는 현재 AllocatedCount에 해당하는 Table값을 주고
	// 2. AllocatedCount에 1을 더한다.
	// 이 사이에 스레드가 겹칠 수 있다.
	ULONG dwResult = ::WaitForSingleObject(m_hSema, INFINITE);
	dwResult = -1;

	if (m_ulAllocatedCount >= m_ulMaxNum) {
		goto RETURN;
	}
	dwResult = m_pulIndexTable[m_ulAllocatedCount];
	m_ulAllocatedCount++;
RETURN:
	::ReleaseSemaphore(m_hSema, 1, NULL);
	return dwResult;
}

void IndexCreator::Free(ULONG _dwIndex)
{
	// 1. Free에서는 AllocatedCount에서 1을 빼고
	// 2. 해당값을 Table 인덱스로 값을 넣는다.
	// 이 사이에 스레드가 겹칠 수 있다.
	ULONG dwResult = ::WaitForSingleObject(m_hSema, INFINITE);
	if (!m_ulAllocatedCount) {
		__debugbreak();
	}
	m_ulAllocatedCount--;
	m_pulIndexTable[m_ulAllocatedCount] = _dwIndex;

	::ReleaseSemaphore(m_hSema, 1, NULL);
}

void IndexCreator::CleanUp()
{
	::CloseHandle(m_hSema);

	if (m_pulIndexTable)
	{
		delete[] m_pulIndexTable;
		m_pulIndexTable = nullptr;
	}
}

void IndexCreator::Check() const
{
	if (m_ulAllocatedCount) {
		// 혹시나 아직 Alloc된 Index가 남아있다면 디버그 브레이크
		__debugbreak();
	}
}

IndexCreator::IndexCreator()
	:m_pulIndexTable(nullptr), m_ulMaxNum(0), m_ulAllocatedCount(0), m_hSema(nullptr)
{
	m_hSema = ::CreateSemaphore(NULL, 1, 1, NULL);
}

IndexCreator::~IndexCreator()
{
	Check();
	CleanUp();
}
