// IndexCreator.h from "megayuchi"

#pragma once

// 적당히 Heap Handle offset을 결정해주는 Index를 관리해주는 클래스이다.
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

