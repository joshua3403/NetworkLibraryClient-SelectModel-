#pragma once
#include "CNetWorkLibrary(MemoryPool).h"

class Network : public joshua::NetworkLibrary
{
public:

public:
	void OnClientJoin(SOCKADDR_IN* sockAddr, UINT64 sessionID);
	virtual void OnClientLeave(UINT64 sessionID);

	// accept����
	// TRUE�� ���� ���
	// FALSE�� ���� ����
	bool OnConnectionRequest(SOCKADDR_IN* sockAddr);

	void OnRecv(UINT64 sessionID, CMessage* message);
	void OnSend(UINT64 sessionID, int sendsize);

	//	virtual void OnWorkerThreadBegin() = 0;                    
	//	virtual void OnWorkerThreadEnd() = 0;                      

	void OnError(int errorcode, WCHAR*);
};