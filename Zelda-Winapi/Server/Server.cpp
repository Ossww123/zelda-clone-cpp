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
#include "ThreadManager.h"
#include "SocketUtils.h"
#include "Listener.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoomManager.h"
#include "DBManager.h"

int main()
{
	SocketUtils::Init();
	GDBManager.Init("game.db");
	GRoomManager.Init();

	ServerServiceRef service = make_shared<ServerService>(
	NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[]() { return make_shared<GameSession>(); },
		100);

	assert(service->Start());

	while (true)
	{
		service->GetIocpCore()->Dispatch(0);
		uint64 now = GetTickCount64();
		GRoomManager.Update(now);

		Sleep(1);
	}



	// ��Ƽ������

	//for (int32 i = 0; i < 5; i++)
	//{
	//	GThreadManager->Launch([=]()
	//		{
	//			while (true)
	//			{
	//				service->GetIocpCore()->Dispatch();
	//			}
	//		});
	//}

	GThreadManager->Join();

	// ���� ����
	SocketUtils::Clear();

}