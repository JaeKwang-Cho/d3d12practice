// IndexCreator.h from "megayuchi"

#pragma once

// 적당히 Heap Handle offset을 결정해주는 Index를 관리해주는 클래스이다.
class IndexCreator
{
public:
	bool Initialize(ULONG _dwNum);
	ULONG Alloc();
	void Free(ULONG _dwIndex);

	void Check() const; 
protected:
private:
	void CleanUp();
public:
protected:
private:
	ULONG* m_pulIndexTable;
	ULONG m_ulMaxNum;
	ULONG m_ulAllocatedCount;

	HANDLE m_hSema;
public:
	IndexCreator();
	~IndexCreator();
};

