// IndexCreator.cpp from "megayuchi"

#include "pch.h"
#include "IndexCreator.h"

bool IndexCreator::Initialize(DWORD _dwNum)
{
	m_pdwIndexTable = new DWORD[_dwNum];
	if (!m_pdwIndexTable) {
		__debugbreak();
		false;
	}
	memset(m_pdwIndexTable, 0, sizeof(DWORD) * _dwNum);
	m_dwMaxNum = _dwNum;

	for (DWORD i = 0; i < m_dwMaxNum; i++) {
		m_pdwIndexTable[i] = i;
	}

	return true;
}

DWORD IndexCreator::Alloc()
{
	// 1. Alloc에서는 현재 AllocatedCount에 해당하는 Table값을 주고
	// 2. AllocatedCount에 1을 더한다.
	// 이 사이에 스레드가 겹칠 수 있다.
	DWORD dwResult = ::WaitForSingleObject(m_hSema, INFINITE);
	dwResult = -1;

	if (m_dwAllocatedCount >= m_dwMaxNum) {
		goto RETURN;
	}
	dwResult = m_pdwIndexTable[m_dwAllocatedCount];
	m_dwAllocatedCount++;

RETURN:
	::ReleaseSemaphore(m_hSema, 1, NULL);
	return dwResult;
}

void IndexCreator::Free(DWORD _dwIndex)
{
	// 1. Free에서는 AllocatedCount에서 1을 빼고
	// 2. 해당값을 Table 인덱스로 값을 넣는다.
	// 이 사이에 스레드가 겹칠 수 있다.
	DWORD dwResult = ::WaitForSingleObject(m_hSema, INFINITE);
	if (!m_dwAllocatedCount) {
		__debugbreak();
	}
	m_dwAllocatedCount--;
	m_pdwIndexTable[m_dwAllocatedCount] = _dwIndex;

	::ReleaseSemaphore(m_hSema, 1, NULL);
}

void IndexCreator::CleanUp()
{
	::CloseHandle(m_hSema);

	if (m_pdwIndexTable)
	{
		delete[] m_pdwIndexTable;
		m_pdwIndexTable = nullptr;
	}
}

void IndexCreator::Check()
{
	if (m_dwAllocatedCount) {
		__debugbreak();
	}
}

IndexCreator::IndexCreator()
	:m_pdwIndexTable(nullptr), m_dwMaxNum(0), m_dwAllocatedCount(0), m_hSema(nullptr)
{
	m_hSema = ::CreateSemaphore(NULL, 1, 1, NULL);
}

IndexCreator::~IndexCreator()
{
	Check();
	CleanUp();
}
