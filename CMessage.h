#pragma once
#include "stdafx.h"
#include "CExceptClass.h"
#include "CMemoryPool(LockFree)_TLS.h"

#define FIXKEY 0x10

class CMessage
{
	/*---------------------------------------------------------------
	Packet Enum.

	----------------------------------------------------------------*/
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 100,		// ��Ŷ�� �⺻ ���� ������.
		eBUFFER_UPSCALE_BYTE = 1,
		eBUFFER_UPSCALE_SHORT = 2,
		eBUFFER_UPSCALE_INT = 4,
		eBUFFER_UPSCALE_INT64 = 8,
		eBUFFER_HEADER_SIZE = 5
	};

private:
	int m_iMaxSize;
	int m_iFront;
	int m_iRear;
	int m_iUsingSize;
	char* m_cpPayloadBuffer;
	char* m_cpHeadPtr;
	LONG m_lRefCount;
	friend class CLFFreeList_TLS<CMessage>;
	CRITICAL_SECTION m_CS;

	BYTE m_bRandKey;

	BOOL m_bIsEncoded;

public:
	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMessage()
	{
		m_iMaxSize = en_PACKET::eBUFFER_DEFAULT;
		m_iFront = m_iRear = m_iUsingSize = m_lRefCount = 0;
		m_cpHeadPtr = (char*)malloc(sizeof(char) * m_iMaxSize);
		m_cpPayloadBuffer = m_cpHeadPtr + 5;
		InitializeCriticalSection(&m_CS);
		m_bIsEncoded = false;
	}

	CMessage(int iBufferSize)
	{
		m_iMaxSize = iBufferSize;
		m_iFront = m_iRear = m_iUsingSize = 0;
		m_cpPayloadBuffer = (char*)malloc(sizeof(char) * m_iMaxSize);
	}

	virtual	~CMessage()
	{
		Release();
	}

	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ  �ı�.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Release(void)
	{
		if (m_cpPayloadBuffer != nullptr)
		{
			free(m_cpPayloadBuffer);
		}
	}

public:
	static CLFFreeList_TLS<CMessage> g_PacketPool;

public:

	void AddRef()
	{
		InterlockedIncrement(&m_lRefCount);
	}

	void SubRef()
	{
		if (InterlockedDecrement(&m_lRefCount) == 0)
		{
			g_PacketPool.Free(this);
		}
	}

	static CMessage* Alloc()
	{
		CMessage* newMessage = g_PacketPool.Alloc();
		newMessage->Clear();
		InterlockedIncrement(&newMessage->m_lRefCount);
		return newMessage;
	}
	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void)
	{
		m_iFront = m_iRear = m_iUsingSize = m_lRefCount = 0;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void) { return m_iMaxSize; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int	GetDataSize(void) { return m_iUsingSize; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ��� ������ ������ ���.
	//
	// Parameters: ����.
	// Return: (int) ��� ������ ������ ������
	//////////////////////////////////////////////////////////////////////////
	int GetFreeSize(void) { return m_iMaxSize - m_iUsingSize; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return m_cpPayloadBuffer; }

	char* GetLanHeaderPtr(void) { return m_cpHeadPtr + 3; }

	char* GetWanHeaderPtr(void) { return m_cpHeadPtr; }


	//////////////////////////////////////////////////////////////////////////
	// Front��ġ ���
	//
	// Parameters: ����.
	// Return: (int) ���� Front�� ��ġ
	//////////////////////////////////////////////////////////////////////////
	int GetFront(void) { return m_iFront; }

	//////////////////////////////////////////////////////////////////////////
	// Rear��ġ ���
	//
	// Parameters: ����.
	// Return: (int) ���� Rear�� ��ġ
	//////////////////////////////////////////////////////////////////////////
	int GetRear(void) { return m_iRear; }

	void PutData(char* data, int size);

	void GetData(char* data, int size);

	static int GetPacketUsingSize()
	{
		return g_PacketPool.GetUseCount();
	}

	void SetLanMessageHeader(char* header, int len);

	void SetEncodingCode();

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize)
	{
		int temp = m_iRear + iSize;
		if (temp < m_iMaxSize)
		{
			m_iRear += iSize;
			temp = iSize;
			m_iUsingSize += iSize;
		}
		else if (temp == m_iMaxSize)
		{
			m_iRear = m_iMaxSize;
			temp = iSize;
			m_iUsingSize = m_iMaxSize;
		}
		else
		{
			temp = temp - m_iMaxSize;
			m_iRear = m_iMaxSize;
			m_iUsingSize = m_iMaxSize;
		}

		return temp;
	}

	int		MoveReadPos(int iSize)
	{
		int temp = m_iFront + iSize;
		if (temp <= m_iRear)
		{
			m_iFront += iSize;
			m_iUsingSize -= iSize;
			temp = iSize;
		}
		else if (temp > m_iRear)
		{
			temp = m_iRear - m_iFront;
			m_iUsingSize -= temp;
		}
		return temp;
	}

	// ���� ����� ������ �ʿ��� ��ŭ Ű��� �Լ�
	void IncreaseBufferSize(int size);

	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	CMessage& operator = (CMessage& clSrCMessage);

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CMessage& operator << (BYTE byValue);
	CMessage& operator << (char chValue);

	CMessage& operator << (short shValue);
	CMessage& operator << (WORD wValue);

	CMessage& operator << (int iValue);
	CMessage& operator << (DWORD dwValue);
	CMessage& operator << (float fValue);

	CMessage& operator << (__int64 iValue);
	CMessage& operator << (double dValue);

	//////////////////////////////////////////////////////////////////////////
// ����.	�� ���� Ÿ�Ը��� ��� ����.
//////////////////////////////////////////////////////////////////////////
	CMessage& operator >> (BYTE& byValue);
	CMessage& operator >> (char& chValue);

	CMessage& operator >> (short& shValue);
	CMessage& operator >> (WORD& wValue);


	CMessage& operator >> (int& iValue);
	CMessage& operator >> (DWORD& dwValue);
	CMessage& operator >> (float& fValue);

	CMessage& operator >> (__int64& iValue);
	CMessage& operator >> (double& dValue);
	CMessage& operator >> (UINT64& dValue);
};


