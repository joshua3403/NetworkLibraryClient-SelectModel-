#include "CNetWorkLibrary(MemoryPool).h"

BOOL joshua::NetworkLibrary::InitialNetwork(const WCHAR* ip, DWORD port, BOOL Nagle)
{
	int retval;
	bool fail = false;
	_bNagle = Nagle;
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LOG(L"SERVER", LOG_ERROR, L"%s\n", L"WSAStartup() Error!");
		return fail;
	}

	ZeroMemory(&_serveraddr, sizeof(SOCKADDR_IN));
	wprintf(L"접속하고자 하는 IP주소를 입력하세요 : ");
	wscanf_s(L"%s", _szIPInput, 32);
	wprintf(L"접속하고자 하는 클라이언트 수를 입력하세요 : ");
	wscanf_s(L"%d", &_ClientCount);
	_serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, _szIPInput, &_serveraddr.sin_addr);
	_serveraddr.sin_port = htons(SERVERPORT);

	CreateSocket();

    return 0;
}

void joshua::NetworkLibrary::CreateSocket()
{
	int retval = 0;
	for (int i = 0; i < _ClientCount; i++)
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
		{
			wprintf(L"socket() eror\n");
			return;
		}

		// 넌블록 소켓으로 전환
		u_long on = 1;
		retval = ioctlsocket(sock, FIONBIO, &on);
		if (retval == SOCKET_ERROR)
		{
			wprintf(L"ioctlsocket() eror\n");
			return;
		}

		// 네이글 알고리즘 해제
		BOOL optval = TRUE;
		setsockopt(sock, IPPROTO_IP, TCP_NODELAY, (char*)&optval, sizeof(optval));

		_SocketArray.push_back(sock);
	}
}

void joshua::NetworkLibrary::ConnectToServer()
{
}
