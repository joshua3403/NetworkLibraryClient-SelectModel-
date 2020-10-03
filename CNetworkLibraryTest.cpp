#include "stdafx.h"
#include "CNetworkLibraryTest.h"

void Network::OnClientJoin(SOCKADDR_IN* sockAddr, UINT64 sessionID)
{
}

void Network::OnClientLeave(UINT64 sessionID)
{
}

bool Network::OnConnectionRequest(SOCKADDR_IN* sockAddr)
{
    return false;
}

void Network::OnRecv(UINT64 sessionID, CMessage* message)
{
}

void Network::OnSend(UINT64 sessionID, int sendsize)
{
}

void Network::OnError(int errorcode, WCHAR*)
{
}
