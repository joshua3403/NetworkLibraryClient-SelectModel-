#include "stdafx.h"
#include "CNetWorkLibrary(MemoryPool).h"

BOOL joshua::NetworkLibrary::InitialNetwork(const WCHAR* ip, DWORD port, BOOL Nagle)
{
	int retval;
	bool fail = false;
	_bNagle = Nagle;
	WSADATA wsa;
	setlocale(LC_ALL, "korean");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LOG(L"SERVER", LOG_ERROR, L"%s\n", L"WSAStartup() Error!");
		return fail;
	}

	ZeroMemory(&_serveraddr, sizeof(SOCKADDR_IN));
	wprintf(L"�����ϰ��� �ϴ� IP�ּҸ� �Է��ϼ��� : ");
	wscanf_s(L"%s", _szIPInput, 32);
	wprintf(L"�����ϰ��� �ϴ� Ŭ���̾�Ʈ ���� �Է��ϼ��� : ");
	wscanf_s(L"%d", &_ClientCount);
	_serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, _szIPInput, &_serveraddr.sin_addr);
	_serveraddr.sin_port = htons(SERVERPORT);


	CreateSocket();

	ConnectToServer();

	CreateThread(0);

    return TRUE;
}

BOOL joshua::NetworkLibrary::CreateThread(DWORD threadCount)
{
	HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, (LPVOID)this, 0, NULL);
	if (hWorkerThread == NULL)
		return FALSE;
	CloseHandle(hWorkerThread);

	HANDLE hConnectThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThread, (LPVOID)this, 0, NULL);
	if (hConnectThread == NULL)
		return FALSE;
	CloseHandle(hConnectThread);
	return 0;
}

void joshua::NetworkLibrary::CreateNewSocket()
{
	int retval = 0;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		LOG(L"SERVER", LOG_ERROR, L"%s\n", L"socket() Error!");
		return;
	}

	// �ͺ�� �������� ��ȯ
	u_long on = 1;
	retval = ioctlsocket(sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)
	{
		LOG(L"SERVER", LOG_ERROR, L"%s\n", L"ioctlsocket() Error!");
		return;
	}

	// ���̱� �˰��� ����
	BOOL optval = TRUE;
	setsockopt(sock, IPPROTO_IP, TCP_NODELAY, (char*)&optval, sizeof(optval));

	_SocketArray.push_back(sock);
}

joshua::st_SESSION* joshua::NetworkLibrary::InsertSession(SOCKET sock, SOCKADDR_IN* sockaddr)
{


		// TCP ���Ͽ��� ���񼼼� ó��
		// ���� ������ ������������ ����(���� ����, Ȥ�� �ܺο��� ������ �Ϻη� ���� ���)�Ǿ� ������ Ŭ���̾�Ʈ����
		// ������ ������� �𸣴� ������ ������ �ǹ� (Close �̺�Ʈ�� ���� ����)
		// TCP KeepAlive���
				// - SO_KEEPALIVE : �ý��� ������Ʈ�� �� ����. �ý����� ��� SOCKET�� ���ؼ� KEEPALIVE ����
		// - SIO_KEEPALIVE_VALS : Ư�� SOCKET�� KEEPALIVE ����
		//tcp_keepalive tcpkl;
		//tcpkl.onoff = TRUE;
		//tcpkl.keepalivetime = 30000; // ms
		//tcpkl.keepaliveinterval = 1000;
		//WSAIoctl(sock, SIO_KEEPALIVE_VALS, &tcpkl, sizeof(tcp_keepalive), 0, 0, NULL, NULL, NULL);
	/*	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&_bNagle, sizeof(_bNagle));*/

		st_SESSION* pSession = new st_SESSION();
		pSession->SessionID = ++_dwSessionID;
		pSession->socket = sock;
		pSession->bIsSend = FALSE;
		pSession->dwPacketCount = 0;
		pSession->SendBuffer.Clear();
		pSession->RecvBuffer.ClearBuffer();
		pSession->lIO->lIOCount = 0;
		pSession->lIO->bIsReleased = FALSE;
		pSession->clientaddr = _serveraddr;
		pSession->IsConnected = TRUE;
		//InterlockedIncrement64(&pSession->lIO->lIOCount);
		InterlockedIncrement64(&_dwSessionCount);
		InterlockedIncrement64(&_dwCount);
		_SessionArray.insert(std::make_pair(sock, pSession));
		return pSession;

}

unsigned int __stdcall joshua::NetworkLibrary::ConnectThread(LPVOID lpParam)
{
	((NetworkLibrary*)lpParam)->ConnectThread();

	return 0;
}

unsigned int __stdcall joshua::NetworkLibrary::WorkerThread(LPVOID lpParam)
{
	((NetworkLibrary*)lpParam)->WorkerThread();
	return 0;
}

void joshua::NetworkLibrary::ConnectThread(void)
{
	while (_bServerOn)
	{
		Sleep(500);
		for (int i = 0; i < _ClientCount; i++)
		{
			CreateNewSocket();
		}

		ConnectToServer();
	}
}

void joshua::NetworkLibrary::WorkerThread(void)
{
	while (_bServerOn)
	{
		st_SESSION* pSession = nullptr;
		FD_SET readSet, writeSet;
		SOCKET UserTable_Socket[FD_SETSIZE];
		int addrlen, retval;
		int iSessionCount = 0;

		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		memset(UserTable_Socket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
		std::map<SOCKET, st_SESSION*>::iterator itor = _SessionArray.begin();
		for (; itor != _SessionArray.end(); )
		{
			pSession = itor->second;
			itor++;

			if (pSession->socket == INVALID_SOCKET)
			{
				// TODO
				//DisconnectSession(pSession->socket);
				continue;
			}
			// Read Set�� Write Set�� ���
			UserTable_Socket[iSessionCount] = pSession->socket;
			FD_SET(pSession->socket, &readSet);
			if (pSession->SendBuffer.GetUsingCount() > 0)
			{
				FD_SET(pSession->socket, &writeSet);
			}

			iSessionCount++;

			if (iSessionCount >= FD_SETSIZE)
			{
				SelectSocket(UserTable_Socket, &readSet, &writeSet);

				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				memset(UserTable_Socket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
				iSessionCount = 0;
			}
		}

		if (iSessionCount > 0)
		{
			SelectSocket(UserTable_Socket, &readSet, &writeSet);
			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			memset(UserTable_Socket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			iSessionCount = 0;
		}

		//ConnectToServer();
	}
	
}

bool joshua::NetworkLibrary::PostSend(SOCKET sock)
{
	return false;
}

bool joshua::NetworkLibrary::PostRecv(SOCKET sock)
{
	st_SESSION* pSession = FindSession(sock);
	int iBuffSize;
	int iResult;


	if (pSession == nullptr)
	{
		wprintf(L"Recv() session is nullptr\n");
	}

	//// ���������� ���� �޽��� Ÿ��
	//pSession->dwRecvTime = timeGetTime();

	// �ޱ� �۾�
	iBuffSize = pSession->RecvBuffer.GetNotBrokenPutSize();
	iResult = recv(pSession->socket, pSession->RecvBuffer.GetWriteBufferPtr(), iBuffSize, 0);
	if (SOCKET_ERROR == iResult)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			DWORD temp = WSAGetLastError();
			WCHAR error[64] = { 0 };
			swprintf_s(error, 64, L"Socket Error UserID : %d\n", temp);
			//PrintError(error);
			// TODO
			//DisconnectSession(clientSock);
			return false;
		}
	}
	else if (iResult == 0)
	{
		// TODO
		//DisconnectSession(clientSock);
		return false;
	}


	if (iResult > 0)
	{
		RecvComplete(pSession, iResult);
	}

	return true;
}

void joshua::NetworkLibrary::SessionRelease(st_SESSION* session)
{
	DisconnectSession(session);

	OnClientLeave(session->SessionID);

	ZeroMemory(&session->clientaddr, sizeof(SOCKADDR_IN));
	CMessage* pPacket = nullptr;
	while (session->SendBuffer.Dequeue(pPacket))
	{
		if (pPacket != nullptr)
			pPacket->SubRef();
		pPacket = nullptr;
	}

	_SessionArray.erase(session->socket);
	InterlockedExchange(&session->bIsSend, FALSE);
	session->SessionID = -1;
	session->socket = INVALID_SOCKET;
	session->dwPacketCount = 0;
	session->lMessageList.clear();
	InterlockedDecrement64(&_lConnectCount);
}

void joshua::NetworkLibrary::DisconnectSocket(SOCKET sock)
{
	LINGER optval;
	int retval;
	optval.l_onoff = 1;
	optval.l_linger = 0;

	retval = setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	closesocket(sock);
}

void joshua::NetworkLibrary::DisconnectSession(st_SESSION* pSession)
{
	LINGER optval;
	int retval;
	optval.l_onoff = 1;
	optval.l_linger = 0;

	retval = setsockopt(pSession->socket, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	closesocket(pSession->socket);
}

void joshua::NetworkLibrary::RecvComplete(st_SESSION* pSession, DWORD dwTransferred)
{
	pSession->RecvBuffer.MoveWritePos(dwTransferred);

	// Header�� Payload ���̸� ���� 2Byte ũ���� Ÿ��
	WORD wHeader;
	while (1)
	{
		// 1. RecvQ�� 'sizeof(Header)'��ŭ �ִ��� Ȯ��
		int iRecvSize = pSession->RecvBuffer.GetUseSize();
		if (iRecvSize < sizeof(wHeader))
		{
			// �� �̻� ó���� ��Ŷ�� ����
			// ����, Header ũ�⺸�� ���� ������ �����Ͱ� �����ȴٸ� 2������ �ɷ���
			break;
		}

		// 2. Packet ���� Ȯ�� : Headerũ��('sizeof(Header)') + Payload����('Header')
		pSession->RecvBuffer.Peek(reinterpret_cast<char*>(&wHeader), sizeof(wHeader));

		if (wHeader != 8)
		{			// Header�� Payload�� ���̰� ������ �ٸ�
			DisconnectSession(pSession);
			LOG(L"SYSTEM", LOG_WARNNING, L"Header Error");
			break;
		}

		if (iRecvSize < sizeof(wHeader) + wHeader)
		{
			// Header�� Payload�� ���̰� ������ �ٸ�
			DisconnectSession(pSession);
			LOG(L"SYSTEM", LOG_WARNNING, L"Header & PayloadLength mismatch");
			break;
		}

		CMessage* pPacket = CMessage::Alloc();

		//// 3. Payload ���� Ȯ�� : PacketBuffer�� �ִ� ũ�⺸�� Payload�� Ŭ ���
		//if (pPacket->GetBufferSize() < wHeader)
		//{
		//	pPacket->SubRef();
		//	DisconnectSession(pSession);
		//	LOG(L"SYSTEM", LOG_WARNNING, L"PacketBufferSize < PayloadSize ");
		//	break;
		//}

		pSession->RecvBuffer.RemoveData(sizeof(wHeader));

		// 4. PacketPool�� Packet ������ �Ҵ�
		if (pSession->RecvBuffer.Peek(pPacket->GetBufferPtr() + sizeof(wHeader), wHeader) != wHeader)
		{
			pPacket->SubRef();
			LOG(L"SYSTEM", LOG_WARNNING, L"RecvQ dequeue error");
			DisconnectSession(pSession);
			break;
		}
		//UINT64 data;
		//memcpy(&data, pPacket->GetBufferPtr() + 2, 8);
		////wprintf(L"Recv data : %08ld\n", data);
		// 5. Packet ó��
		pSession->RecvBuffer.RemoveData(wHeader);
		InterlockedIncrement64(&_lRecvTPS);

		OnRecv(pSession->SessionID, pPacket);

		pPacket->SubRef();
	}
	SessionRelease(pSession);
	//CreateNewSocket();
	//PostRecv(pSession->socket);
}

void joshua::NetworkLibrary::CreateSocket()
{
	int retval = 0;
	for (int i = 0; i < _ClientCount; i++)
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
		{
			LOG(L"SERVER", LOG_ERROR, L"%s\n", L"socket() Error!");
			return;
		}

		// �ͺ�� �������� ��ȯ
		u_long on = 1;
		retval = ioctlsocket(sock, FIONBIO, &on);
		if (retval == SOCKET_ERROR)
		{
			LOG(L"SERVER", LOG_ERROR, L"%s\n", L"ioctlsocket() Error!");
			return;
		}

		// ���̱� �˰��� ����
		BOOL optval = TRUE;
		setsockopt(sock, IPPROTO_IP, TCP_NODELAY, (char*)&optval, sizeof(optval));

		_SocketArray.push_back(sock);
	}
}

void joshua::NetworkLibrary::ConnectToServer()
{
	FD_SET writeSet, errorSet;
	SOCKET UserTable_Socket[FD_SETSIZE];
	int addrlen, retval;
	int iSessionCount = 0;

	FD_ZERO(&errorSet);
	FD_ZERO(&writeSet);
	memset(UserTable_Socket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);

	SOCKET sock;

	for (std::list<SOCKET>::iterator itor = _SocketArray.begin(); itor!= _SocketArray.end(); itor++)
	{
		if ((*itor) == INVALID_SOCKET)
			continue;
		sock = (*itor);
		retval = connect(sock, (SOCKADDR*)&_serveraddr, sizeof(_serveraddr));
		//if (retval == 0)
		//{
		//	// Read Set�� Write Set�� ���
		UserTable_Socket[iSessionCount] = sock;
		FD_SET(sock, &writeSet);
		iSessionCount++;

		if (retval == SOCKET_ERROR)
		{
			DWORD Error = WSAGetLastError();
			if (Error == WSAEWOULDBLOCK)
				continue;
			else
			{
				_lConnectFailCount++;
				_SocketArray.erase(itor);
				LOG(L"SERVER", LOG_ERROR, L"%s\n", L"connect() Error!");
			}
		}

		if (iSessionCount >= FD_SETSIZE)
		{
			SelectConnect(UserTable_Socket, &writeSet, &errorSet);

			FD_ZERO(&errorSet);
			FD_ZERO(&writeSet);
			memset(UserTable_Socket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			iSessionCount = 0;
		}
	}

	if (iSessionCount >= 0)
	{
		SelectConnect(UserTable_Socket, &writeSet, &errorSet);

		FD_ZERO(&errorSet);
		FD_ZERO(&writeSet);
		memset(UserTable_Socket, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
		iSessionCount = 0;
	}
}

void joshua::NetworkLibrary::SelectConnect(SOCKET* pTableSocket, FD_SET* pWriteSet, FD_SET* pErrorSet)
{
	int iResult = 0;
	int iCnt = 0;
	BOOL bProcFlag;

	iResult = select(0, 0, pWriteSet, pErrorSet, &_SelectTime);
	if (iResult > 0)
	{
		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (pTableSocket[i] == INVALID_SOCKET)
				continue;

			if (FD_ISSET(pTableSocket[i], pWriteSet))
			{
				// ���� ����, ���ǰ� Ŭ���̾�Ʈ ����
				st_SESSION* newSession = InsertSession(pTableSocket[i], &_serveraddr);
				OnClientJoin(&_serveraddr, newSession->SessionID);
				PostRecv(pTableSocket[i]);
				InterlockedIncrement64(&_lConnectCount);
				continue;
			}

			if (FD_ISSET(pTableSocket[i], pErrorSet))
			{
				// ���� ����
				DWORD error = WSAGetLastError();
				LOG(L"SERVER", LOG_ERROR, L"%s\n", L"connect() Error code : %d!", error);
				_lConnectFailCount++;
			}
		}

		for (std::list<SOCKET>::iterator itor = _SocketArray.begin(); itor != _SocketArray.end();)
		{
			DisconnectSocket(*itor);
			itor = _SocketArray.erase(itor);
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		wprintf(L"Select() Error!\n");
	}
}

void joshua::NetworkLibrary::SelectSocket(SOCKET* pTableSocket, FD_SET* pReadSet, FD_SET* pWriteSet)
{
	int iResult = 0;
	int iCnt = 0;
	BOOL bProcFlag;

	iResult = select(0, pReadSet, pWriteSet, 0, &_SelectTime);

	if (iResult > 0)
	{
		for (int i = 0; i < FD_SETSIZE; i++)
		{
			bProcFlag = TRUE;
			if (pTableSocket[i] == INVALID_SOCKET)
				continue;

			// FD_WRITE üũ
			if (FD_ISSET(pTableSocket[i], pWriteSet))
			{
				--iResult;
				bProcFlag = PostRecv(pTableSocket[i]);
			}

			if (FD_ISSET(pTableSocket[i], pReadSet))
			{
				PostSend(pTableSocket[i]);
			}
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		//PrintError(L"Select() Error!");
	}
}

joshua::st_SESSION* joshua::NetworkLibrary::FindSession(SOCKET sock)
{
	return _SessionArray.find(sock)->second;
}

BOOL joshua::NetworkLibrary::Start(DWORD port, BOOL nagle, const WCHAR* ip, DWORD threadCount, __int64 MaxClient)
{	// ���� �ʱ�ȭ
	_dwSessionMax = MaxClient;
	_bServerOn = TRUE;

	if (InitialNetwork(ip, port, nagle) == false)
	{
		LOG(L"SERVER", LOG_ERROR, L"%s\n", L"Initialize failed!");
		return FALSE;
	}
	return TRUE;
}

void joshua::NetworkLibrary::PrintPacketCount()
{
	wprintf(L"==================== IOCP Echo Server Test ====================\n");
	wprintf(L" - ConnectCount : %lld\n", _lConnectCount);
	//wprintf(L" - PacketAlloc : %08d\n", CMessage::GetPacketAllocSize());
	wprintf(L" - PacketUse : %08d\n", CMessage::GetPacketUsingSize());
	wprintf(L"===============================================================\n");
}
