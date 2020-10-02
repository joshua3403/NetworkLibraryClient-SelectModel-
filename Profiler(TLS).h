#pragma once
#include "stdafx.h"

enum e_DEF_PROFILER
{
	e_MAX_NAME_LEN = 64,
	e_MAX_NODE_COUNT = 100,
	e_MAX_THREAD_COUNT = 50
};


typedef struct st_NODE
{
	BOOL			bIsUse;
	WCHAR			szName[e_MAX_NAME_LEN];			// �������� ���� �̸�.

	LARGE_INTEGER	lStartTime;			// �������� ���� ���� �ð�.

	__int64			iTotalTime;			// ��ü ���ð� ī���� Time.	(��½� ȣ��ȸ���� ������ ��� ����)
	__int64			iMin[2];			// �ּ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
	__int64			iMax[2];			// �ִ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] ���� �ִ� [2])
	
	unsigned long			iCall;				// ���� ȣ�� Ƚ��
}NODE;

// st_NODE�� ������ ����ü
typedef struct st_THREAD_NODE
{
	DWORD dwThreadID;
	NODE* pNode;
}THREAD_NODE;

// �ʱ�ȭ �Լ�
void InitialProfiler();

// �������ϸ� ���� �Լ�
bool ProfilingBegin(WCHAR* szName);

// �������ϸ� ��带 ���ϴ� �Լ�
bool GetNode(WCHAR* szName, NODE** outNode);

bool ProfilingEnd(WCHAR* szName);

bool ProfilePrint(void);

#define PRO_BEGIN(X) ProfilingBegin(X)
#define PRO_END(X) ProfilingEnd(X)