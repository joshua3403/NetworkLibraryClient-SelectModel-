#pragma once
#include "stdafx.h"
#include "CCrashDumpClass.h"
#include "CLog.h"
#include "MemoryPool(LockFree).h"
#include "Profiler(TLS).h"

#define MAXDUMPCOUNT 500
#define CHECKCODE 0x0000000019921107

template<class DATA>
class CLFFreeList_TLS
{
private:
	class CDataDump;
	struct  st_DataDump_Node
	{
		//__declspec(align(64))
		CDataDump* pDataDump;
		DATA Data;
	};

public:
	CLFFreeList_TLS(BOOL bPlacementNew = false);
	virtual ~CLFFreeList_TLS();
	DATA* Alloc();
	bool Free(DATA* data);

	int GetUseCount() { return m_dwUseCount; }
	int GetAllocCount() { return m_pDataDump->GetAllocCount(); }

private:
	CDataDump* DataDumpAlloc()
	{
		CDataDump* DataDump = m_pDataDump->Alloc();

		if (DataDump->m_pFreeList == nullptr)
		{
			DataDump->Initial(this);
		}
		else
			DataDump->Clear();

		TlsSetValue(m_dwTlsIndex, DataDump);

		return DataDump;
	}

private:
	DWORD m_dwTlsIndex;

	DWORD m_dwUseCount;

	CLFFreeList<CDataDump>* m_pDataDump;
	

private:

	class CDataDump
	{
	public:
		CDataDump();
		virtual ~CDataDump() { delete[] m_pDataDumpNode; };
		void Initial(CLFFreeList_TLS<DATA>* MemoryPool);
		DATA* Alloc();
		bool Clear();
		bool Free();
	private:
		friend class CLFFreeList_TLS<DATA>;
		CLFFreeList_TLS<DATA>* m_pFreeList;
		st_DataDump_Node* m_pDataDumpNode;
		// 노드 덤프 내의 Data 개수
		DWORD m_dwNodeCount;
		// 총 데이터의 개수 중에서 할당된 개수
		DWORD m_dwUsingCount;
		// 총 데이터의 개수 중에서 남은 개수
		DWORD m_dwFreeCount;

	};
};

template<class DATA>
CLFFreeList_TLS<DATA>::CDataDump::CDataDump()
{
	m_pFreeList = nullptr;
	m_dwNodeCount = MAXDUMPCOUNT;
	m_dwUsingCount = 0;
	m_dwFreeCount = MAXDUMPCOUNT;
}

template<class DATA>
void CLFFreeList_TLS<DATA>::CDataDump::Initial(CLFFreeList_TLS<DATA>* MemoryPool)
{
	m_pFreeList = MemoryPool;
	m_dwFreeCount = MAXDUMPCOUNT;
	m_dwNodeCount = MAXDUMPCOUNT;
	m_dwUsingCount = 0;

	m_pDataDumpNode = (st_DataDump_Node*)malloc(sizeof(st_DataDump_Node) * MAXDUMPCOUNT);
	for (int i = 0; i < m_dwNodeCount; i++)
	{
		m_pDataDumpNode[i].pDataDump = this;
	}
}

template<class DATA>
DATA* CLFFreeList_TLS<DATA>::CDataDump::Alloc()
{
	DATA* allocData = &m_pDataDumpNode[m_dwUsingCount++].Data;
	if (MAXDUMPCOUNT ==  m_dwUsingCount)
		m_pFreeList->DataDumpAlloc();
	return allocData;
}

template<class DATA>
bool CLFFreeList_TLS<DATA>::CDataDump::Clear()
{
	m_dwFreeCount = MAXDUMPCOUNT;
	m_dwUsingCount = 0;
	return true;
}

template<class DATA>
bool CLFFreeList_TLS<DATA>::CDataDump::Free()
{
	if ((--m_dwFreeCount) == 0)
	{
		m_pFreeList->m_pDataDump->Free(m_pDataDumpNode->pDataDump);

		return true;
	}
	return false;
}

template<class DATA>
CLFFreeList_TLS<DATA>::CLFFreeList_TLS(BOOL bPlacementNew)
{
	m_dwTlsIndex = TlsAlloc();
	if (m_dwTlsIndex == TLS_OUT_OF_INDEXES)
		CRASH();

	m_pDataDump = new CLFFreeList<CDataDump>(100);
	m_dwUseCount = 0;
}

template<class DATA>
CLFFreeList_TLS<DATA>::~CLFFreeList_TLS()
{
	TlsFree(m_dwTlsIndex);
	delete[] m_pDataDump;
}

template<class DATA>
DATA* CLFFreeList_TLS<DATA>::Alloc()
{
	CDataDump* DataDump = (CDataDump*)TlsGetValue(m_dwTlsIndex);
	if (DataDump == nullptr)
	{
		DataDump = DataDumpAlloc();
	}
	InterlockedIncrement(&m_dwUseCount);
	PRO_BEGIN(_T("Chunk Alloc()"));
	DATA* data = DataDump->Alloc();
	PRO_END(_T("Chunk Alloc()"));

	return data;
}

template<class DATA>
bool CLFFreeList_TLS<DATA>::Free(DATA* data)
{
	st_DataDump_Node* DataDump = (st_DataDump_Node*)((char*)data - 8);

	InterlockedDecrement(&m_dwUseCount);
	DataDump->pDataDump->Free();

	return true;
}




