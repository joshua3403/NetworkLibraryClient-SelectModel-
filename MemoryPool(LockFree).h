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
class CLFFreeList
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
		}

		st_BLOCK_NODE *stpNextBlock;
	};


	struct st_TOP_NODE
	{
		st_BLOCK_NODE* pTopNode;
		LONG64 lCount;
	};
public:

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CLFFreeList(int iBlockNum = 1000, bool bPlacementNew = false);
	virtual	~CLFFreeList();


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
	LONG64		GetAllocCount(void) { return m_lMaxCount; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	LONG64		GetUseCount(void) { return m_lUseCount; }


public:

private :
	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.

	st_TOP_NODE* _pTop;
	LONG64 m_lFreeCount;
	LONG64 m_lMaxCount;
	LONG64 m_lUseCount;
	bool  m_bUsingPlacementNew;
	LONG64 m_lCount;
};



template<class DATA>
inline CLFFreeList<DATA>::CLFFreeList(int iBlockNum, bool bPlacementNew)
{
	// ������ ������ ��ü ��
	DATA* newObject = nullptr;
	st_BLOCK_NODE* newNode = nullptr;

	// �ɹ� ���� �ʱ�ȭ
	m_lMaxCount = m_lFreeCount = iBlockNum;
	m_lUseCount = 0;
	_pTop = (st_TOP_NODE*)_aligned_malloc(sizeof(st_TOP_NODE), 16);
	_pTop->pTopNode = nullptr;
	_pTop->lCount = 0;
	m_lCount = 0;
	m_bUsingPlacementNew = bPlacementNew;
	// ��������
	int count = m_lMaxCount;

	if (m_lMaxCount != 0)
	{
		while (count > 0)
		{
			void* newBlock = malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
			newNode = new(newBlock) st_BLOCK_NODE;
			newNode->stpNextBlock = _pTop->pTopNode;
			_pTop->pTopNode = newNode;
			if (m_bUsingPlacementNew)
			{
				newObject = new((char*)newBlock + sizeof(st_BLOCK_NODE)) DATA;
			}
			count--;
			//wprintf(L"CFreeList() : MemoryPool Header Pointer : %p, newNode Pointer : %p newObject Pointer : %p\n", _pTop->pTopNode, newNode, newObject);

		}
	}
}

template<class DATA>
inline CLFFreeList<DATA>::~CLFFreeList()
{
	st_BLOCK_NODE* temp;

	for (int i = 0; i < m_lMaxCount; i++)
	{
		temp = _pTop->pTopNode;
		if (temp != nullptr)
		{
			_pTop->pTopNode = _pTop->pTopNode->stpNextBlock;
			free(temp);
		}

	}

	_aligned_free(_pTop);
}

template<class DATA>
inline DATA* CLFFreeList<DATA>::Alloc(void)
{
	// ������ ������ ��ü ��
	DATA* newObject = nullptr;
	st_BLOCK_NODE* newNode = nullptr;
	st_TOP_NODE CloneTop;

	LONG64 MaxCount = m_lMaxCount;
	InterlockedIncrement64(&m_lUseCount);
	// ���� ������ �Ѵٸ�
	if (MaxCount < m_lUseCount)
	{
		newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
		InterlockedIncrement64(&m_lMaxCount);

		newObject = (DATA*)(newNode + 1);
		new (newObject)DATA;
		return newObject;
	}
	else
	{
		LONG64 newCount = InterlockedIncrement64(&m_lCount);
		do
		{
			CloneTop.pTopNode = _pTop->pTopNode;
			CloneTop.lCount = _pTop->lCount;
		} while (!InterlockedCompareExchange128((LONG64*)_pTop, newCount, (LONG64)_pTop->pTopNode->stpNextBlock, (LONG64*)&CloneTop));

		newNode = CloneTop.pTopNode;
	}

	newObject = (DATA*)(newNode + 1);

	if (m_bUsingPlacementNew)
		new (newObject)DATA;

	return newObject;
}

template<class DATA>
inline bool CLFFreeList<DATA>::Free(DATA* pData)
{
	st_BLOCK_NODE* returnedBlock = ((st_BLOCK_NODE*)pData) - 1;
	st_TOP_NODE CloneTop;

	LONG64 newCount = InterlockedIncrement64(&m_lCount);
	do
	{
		CloneTop.pTopNode = _pTop->pTopNode;
		CloneTop.lCount = _pTop->lCount;
		returnedBlock->stpNextBlock = _pTop->pTopNode;
	} while (!InterlockedCompareExchange128((LONG64*)_pTop, newCount, (LONG64)returnedBlock, (LONG64*)&CloneTop));

	if (m_bUsingPlacementNew)
		pData->~DATA();

	InterlockedDecrement64(&m_lUseCount);

	return true;

}


