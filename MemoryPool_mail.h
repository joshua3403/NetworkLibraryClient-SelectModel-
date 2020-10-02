/*---------------------------------------------------------------

	procademy MemoryPool.

	�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ����Ÿ(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);

				
----------------------------------------------------------------*/
#pragma once
#include "stdafx.h"


template <class DATA>
class CFreeList
{
private:

	/* **************************************************************** */
	// �� �� �տ� ���� ��� ����ü.
	/* **************************************************************** */
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			stpNextBlock = nullptr;
			check = 0x1107;
			//wprintf(L"st_BLOCK_NODE Contructor\n");
		}

		~st_BLOCK_NODE()
		{
			//wprintf(L"st_BLOCK_NODE Destroyer\n");
		}
		DWORD check;
		st_BLOCK_NODE *stpNextBlock;
	};

public:

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
			CFreeList(int iBlockNum, bool bPlacementNew = false);
	virtual	~CFreeList();


	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA	*Alloc(void);

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA *pData);


	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetAllocCount(void) { return m_iMaxCount; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return m_iUseCount; }


public:
	//friend class CMessage;

private :
	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.

	st_BLOCK_NODE* _pFreeNode;
	int m_iFreeCount;
	int m_iMaxCount;
	int m_iUseCount;
	bool  m_bUsingPlacementNew;
	CRITICAL_SECTION m_CS;

};

template<class DATA>
inline CFreeList<DATA>::CFreeList(int iBlockNum, bool bPlacementNew)
{
	// ������ ������ ��ü ��
	DATA* newObject = nullptr;
	st_BLOCK_NODE* newNode = nullptr;

	// �ɹ� ���� �ʱ�ȭ
	m_iMaxCount = m_iFreeCount = iBlockNum;
	m_iUseCount = 0;
	_pFreeNode = new st_BLOCK_NODE;
	m_bUsingPlacementNew = bPlacementNew;
	InitializeCriticalSection(&m_CS);
	//wprintf(L"%d\n", sizeof(st_BLOCK_NODE));
	// ��������
	int count = m_iMaxCount;

	if (m_iMaxCount != 0)
	{

		while (count > 0)
		{
			void* newBlock = malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
			newNode = new(newBlock) st_BLOCK_NODE;
			newNode->stpNextBlock = this->_pFreeNode->stpNextBlock;
			this->_pFreeNode->stpNextBlock = newNode;
			count--;
			//wprintf(L"CFreeList() : MemoryPool Header Pointer : %p, newNode Pointer : %p newObject Pointer : %p\n", this->_pFreeNode->stpNextBlock, newNode, newObject);

		}
	}
}

template<class DATA>
inline CFreeList<DATA>::~CFreeList()
{
	wprintf(L"~CFreeList()\n");
	int count = m_iMaxCount - m_iUseCount;
	void* temp = this->_pFreeNode->stpNextBlock;
	void* next = nullptr;

	while (count > 0)
	{
		wprintf(L"~CFreeList() : MemoryPool Header Pointer : %p\n", temp);
		next = (*(st_BLOCK_NODE*)temp).stpNextBlock;
		free(temp);
		temp = next;
		count--;

	}
}

template<class DATA>
inline DATA* CFreeList<DATA>::Alloc(void)
{
	EnterCriticalSection(&m_CS);
	// ������ ������ ��ü ��
	DATA* newObject = nullptr;
	st_BLOCK_NODE* newNode = nullptr;

	// ���� ��밡���� �� Ȯ��
	int FreeCount = m_iMaxCount - m_iUseCount;

	// �޸� ���� Ȯ��
	if (FreeCount <= 0)
	{
		if (m_bUsingPlacementNew)
		{
			void* newBlock = malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
			newNode = new(newBlock) st_BLOCK_NODE;
			newNode->stpNextBlock = this->_pFreeNode->stpNextBlock;
			this->_pFreeNode->stpNextBlock = newNode;
			newObject = new((char*)newBlock + sizeof(st_BLOCK_NODE)) DATA;
		}
		else
		{
			void* newBlock = malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
			newNode = new(newBlock) st_BLOCK_NODE;
			newNode->stpNextBlock = this->_pFreeNode->stpNextBlock;
			this->_pFreeNode->stpNextBlock = newNode;
			newObject = new((char*)newBlock + sizeof(st_BLOCK_NODE)) DATA;
		}
		m_iMaxCount++;
	}

	void* blockPointer = this->_pFreeNode->stpNextBlock;
	// �� ������ ��ȯ
	if (m_bUsingPlacementNew)
	{
		newObject = new((char*)blockPointer + sizeof(st_BLOCK_NODE)) DATA;
	}
	this->_pFreeNode->stpNextBlock = (*(st_BLOCK_NODE*)blockPointer).stpNextBlock;
	//wprintf(L"Alloc() : MemoryPool Header Pointer : %p\n", this->_pFreeNode->stpNextBlock);
	m_iUseCount++;
	LeaveCriticalSection(&m_CS);
	return (DATA*)((char*)blockPointer + sizeof(st_BLOCK_NODE));
}

template<class DATA>
inline bool CFreeList<DATA>::Free(DATA* pData)
{
	EnterCriticalSection(&m_CS);
	void* returnedBlock = (char*)pData - sizeof(st_BLOCK_NODE);
	if ((*(st_BLOCK_NODE*)returnedBlock).check != 0x1107)
	{
		return false;
	}
	if (m_bUsingPlacementNew)
	{
		pData->~DATA();
	}
	(*(st_BLOCK_NODE*)returnedBlock).stpNextBlock = this->_pFreeNode->stpNextBlock;
	this->_pFreeNode->stpNextBlock = (st_BLOCK_NODE*)returnedBlock;
	m_iUseCount--;
	LeaveCriticalSection(&m_CS);
	return true;
}


