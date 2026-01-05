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

// Select 모델 = (select 함수)

// socket set
// 1) 읽기[] 쓰기[] 예외[] 관찰 대상
// 2) select(readSet, writeSet, exceptSet); -> 관찰 시작
// 3) 적어도 하나의 소켓 준비되면 리턴 -> 낙오자는 알아서 제거
// 4) 남은 소켓 체크해서 진행

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

	vector<Session> sessions;
	sessions.reserve(100);

	fd_set reads;
	fd_set writes;

	while (true)
	{
		// 소켓 셋 초기화
		FD_ZERO(&reads);

		// ListenSocket 등록
		FD_SET(listenSocket, &reads);

		// 소켓 등록
		for (Session& s : sessions)
			FD_SET(s.socket, &reads);

		// [옵션] 마지막 timeout 인자 설정 가능
		int32 retVal = ::select(0, &reads, nullptr, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
			break;

		if (FD_ISSET(listenSocket, &reads))
		{
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			if (clientSocket != INVALID_SOCKET)
			{
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				cout << "Client Connected" << endl;
				sessions.push_back(Session{ clientSocket });
			}
		}

		// 나머지 소켓 체크
		for (Session& s : sessions)
		{
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUF_SIZE, 0);
				if (recvLen = 0)
				{
					continue;
				}

				cout << "RecvData = " << s.recvBuffer << endl;
				cout << "RecvLen = " << recvLen << endl;
			}
		}
	}
	
	SocketUtils::Clear();
}