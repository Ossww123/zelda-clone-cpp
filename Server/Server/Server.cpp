#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <map>
#include <queue>
using namespace std;

// WSAEventSelect - WSAEventSelect가 핵심이 되는
// 소켓과 관련된 네트워크 이벤트를 [이벤트 객체]를 통해 감지

// 생성 : WSACreateEvent (수동 리셋 + Manual-Reset + Non-Signasted 상태 시작)
// 삭제 : WsaCloswEvent
// 신호 상태 감지 : WSAWaitForMultipleEvents
// 구체적인 네트워크 이벤트 알아내기 : WSAEnumNetworkEvents

const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};

int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	// 논블로킹
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;

	SOCKADDR_IN clientAddr;
	int32 addrLen = sizeof(clientAddr);

	vector<WSAEVENT> wsaEvents;
	vector<Session> sessions;
	sessions.reserve(100);

	WSAEVENT listenEvent = ::WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{ listenSocket });

	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true)
	{
		int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED)
			continue;

		index -= WSA_WAIT_EVENT_0;

		WSANETWORKEVENTS networkEvents;
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
		continue;

		if (networkEvents.lNetworkEvents & FD_ACCEPT)
		{
			// Error-Check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);

			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
			{
				cout << "Client Connected" << endl;

				WSAEVENT clientEvent = ::WSACreateEvent();
				wsaEvents.push_back(clientEvent);
				sessions.push_back(Session{ clientSocket });
				if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
					return 0;
			}
		}

		// Client Session 소켓 체크
		if (networkEvents.lNetworkEvents & FD_READ)
		{
			if (networkEvents.iErrorCode[FD_READ_BIT] != 0)
				continue;

			Session& s = sessions[index];

			// Read
			int32 recvLen = ::recv(s.socket, s.recvBuffer, BUF_SIZE, 0);
			if (recvLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
			{
				if (recvLen <= 0)
					continue;
			}

			cout << "Recv Data = " << s.recvBuffer << endl;
			cout << "RecvLen = " << recvLen << endl;
		}
	}
	
	SocketUtils::Clear();
}