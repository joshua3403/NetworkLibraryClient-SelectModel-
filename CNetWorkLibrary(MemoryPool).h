#pragma once
#include "stdafx.h"
#include "CNewRingBuffer.h"
#include "Stack(LockFree).h"
#include "Queue(LockFree).h"
#include "CMessage.h"
#include "CLog.h"

#define READ 3
#define WRITE 5
#define MAX_CLIENT_COUNT 1000
#define SERVERPORT 6000

// sizeof(UINT64) == 8 Byte == 64 Bit
// [00000000 00000000 0000] [0000 00000000 00000000 00000000 00000000 00000000]
// 1. ���� 2.5 Byte = Index ����
// 2. ���� 5.5 Byte = SessionID ����
#define CreateSessionID(ID, Index)	(((UINT64)Index << 44) | ID)				
#define GetSessionIndex(SessionID)	((SessionID >> 44) & 0xfffff)
#define GetSessionID(SessionID)		(SessionID & 0x00000fffffffffff)
namespace joshua
{
	struct st_SESSION_FLAG
	{

		st_SESSION_FLAG(UINT64 ioCount, UINT64 isReleased)
		{
			lIOCount = ioCount;
			bIsReleased = isReleased;
		}
		LONG64		lIOCount;
		LONG64		bIsReleased;
	};



	struct st_SESSION
	{
		int index;
		UINT64 SessionID;
		SOCKET socket;
		SOCKADDR_IN clientaddr;
		CQueue<CMessage*> SendBuffer;
		RingBuffer RecvBuffer;
		OVERLAPPED SendOverlapped;
		OVERLAPPED RecvOverlapped;
		LONG bIsSend;
		st_SESSION_FLAG* lIO;

		DWORD dwPacketCount;
		std::list<CMessage*> lMessageList;

		LONG64 Tps;


		// SessionID�� 0�̸� ������� �ʴ� ����
		st_SESSION()
		{
			dwPacketCount = 0;
			SessionID = -1;
			bIsSend = FALSE;
			socket = INVALID_SOCKET;
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			index = 0;
			SendBuffer.Clear();
			RecvBuffer.ClearBuffer();
			lIO = (st_SESSION_FLAG*)_aligned_malloc(sizeof(st_SESSION_FLAG), 16);
			lIO->bIsReleased = FALSE;
			lIO->lIOCount = 0;
		}

		~st_SESSION()
		{
			_aligned_free(lIO);
		}

	};


	class NetworkLibrary
	{
	private:
		SOCKET _slisten_socket;
		BOOL _bServerOn;
		BOOL _bNagle;

		// ����� �Ϸ� ��Ʈ ����// IOCP ����
		HANDLE _hCP;

		LONG64 _dwSessionCount;
		LONG64 _dwSessionMax;
		LONG64 _dwCount;

		// Thread
		HANDLE _AcceptThread;
		int _iThreadCount;
		std::vector<HANDLE> _ThreadVector;

		// Session
		SOCKADDR_IN _serveraddr;
		UINT64 _dwSessionID;
		std::vector<st_SESSION> _SessionArray;
		std::vector<SOCKET> _SocketArray;
		std::stack<UINT64> _ArrayIndex;
		SRWLOCK _SessionLock;

		LONG64 _lSendTPS;
		LONG64 _lRecvTPS;
		LONG64 _lAcceptTPS;
		LONG64 _lAcceptCount;

		WCHAR _szIPInput[32];
		UINT64 _ClientCount;


	private:
		// ���� �ʱ�ȭ
		BOOL InitialNetwork(const WCHAR* ip, DWORD port, BOOL Nagle);
		// ������ ����
		// 0�̸� ���μ��� * 2
		BOOL CreateThread(DWORD threadCount);
		// ���� �Ҵ� �� ����
		BOOL CreateSession();

		void PushIndex(UINT64 index);

		UINT64 PopIndex();

		// �� ���� ������ ���� ������ ���� ����
		st_SESSION* InsertSession(SOCKET sock, SOCKADDR_IN* sockaddr);

		// Accept�� ������ �������� ���� �Լ�
		static unsigned int WINAPI AcceptThread(LPVOID lpParam);
		static unsigned int WINAPI WorkerThread(LPVOID lpParam);

		void AcceptThread(void);
		void WorkerThread(void);

		bool PostSend(st_SESSION* session);
		bool PostRecv(st_SESSION* session);

		void SessionRelease(st_SESSION* session);
		void DisconnectSocket(SOCKET sock);
		void DisconnectSession(st_SESSION* pSession);
		joshua::st_SESSION* SessionReleaseCheck(UINT64 iSessionID);

		// IOCP completion notice
		void RecvComplete(st_SESSION* pSession, DWORD dwTransferred);
		void SendComplete(st_SESSION* pSession, DWORD dwTransferred);

		void CreateSocket();

		void ConnectToServer();

	protected:
		// Accept�� ���� ó�� �Ϸ��� ȣ���ϴ� �Լ�
		virtual void OnClientJoin(SOCKADDR_IN* sockAddr, UINT64 sessionID) = 0;
		virtual void OnClientLeave(UINT64 sessionID) = 0;

		// accept����
		// TRUE�� ���� ���
		// FALSE�� ���� ����
		virtual bool OnConnectionRequest(SOCKADDR_IN* sockAddr) = 0;

		virtual void OnRecv(UINT64 sessionID, CMessage* message) = 0;
		virtual void OnSend(UINT64 sessionID, int sendsize) = 0;

		//	virtual void OnWorkerThreadBegin() = 0;                    
		//	virtual void OnWorkerThreadEnd() = 0;                      

		virtual void OnError(int errorcode, WCHAR*) = 0;
		 
		void SendPacket(UINT64 id, CMessage* message);
		bool Dissconnect(UINT64 id, CMessage* message);
	public:

		NetworkLibrary()
		{
			_slisten_socket = INVALID_SOCKET;
			_dwSessionID = 0;
			_dwSessionCount = _dwSessionMax = 0;
			_hCP = INVALID_HANDLE_VALUE;
			ZeroMemory(&_serveraddr, sizeof(_serveraddr));
			_bNagle = FALSE;
			_dwCount = 0;
			_iThreadCount = 0;
			_lSendTPS = 0;
			_lRecvTPS = 0;
			_lAcceptTPS = 0;
			_lAcceptCount = 0;
			InitializeSRWLock(&_SessionLock);
		}
		
		~NetworkLibrary()
		{
			closesocket(_slisten_socket);
			CloseHandle(_hCP);
			WSACleanup();
		}

		BOOL Start(DWORD port, BOOL nagle, const WCHAR* ip = nullptr, DWORD threadCount = 0, __int64 MaxClient = 0);

		void Stop();

		void ReStart();

		void PrintPacketCount();


	};
}
