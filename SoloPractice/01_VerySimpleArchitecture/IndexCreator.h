// IndexCreator.h from "megayuchi"

#pragma once

// ������ Heap Handle offset�� �������ִ� Index�� �������ִ� Ŭ�����̴�.
class IndexCreator
{
public:
	bool Initialize(DWORD _dwNum);
	DWORD Alloc();
	void Free(DWORD _dwIndex);

	void Check();
protected:
private:
	void CleanUp();
public:
protected:
private:
	DWORD* m_pdwIndexTable;
	DWORD m_dwMaxNum;
	DWORD m_dwAllocatedCount;

	HANDLE m_hSema;
public:
	IndexCreator();
	~IndexCreator();
};

