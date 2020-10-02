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
	wprintf(L"�����ϰ��� �ϴ� IP�ּҸ� �Է��ϼ��� : ");
	wscanf_s(L"%s", _szIPInput, 32);
	wprintf(L"�����ϰ��� �ϴ� Ŭ���̾�Ʈ ���� �Է��ϼ��� : ");
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

		// �ͺ�� �������� ��ȯ
		u_long on = 1;
		retval = ioctlsocket(sock, FIONBIO, &on);
		if (retval == SOCKET_ERROR)
		{
			wprintf(L"ioctlsocket() eror\n");
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
}
