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
	// 1. Alloc������ ���� AllocatedCount�� �ش��ϴ� Table���� �ְ�
	// 2. AllocatedCount�� 1�� ���Ѵ�.
	// �� ���̿� �����尡 ��ĥ �� �ִ�.
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
	// 1. Free������ AllocatedCount���� 1�� ����
	// 2. �ش簪�� Table �ε����� ���� �ִ´�.
	// �� ���̿� �����尡 ��ĥ �� �ִ�.
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
