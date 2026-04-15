#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>
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
#include "ChatConnector.h"
#include "RedisClient.h"
#include "RedisSubscriber.h"

static bool IsMultiMode(int argc, char* argv[])
{
	if (argc < 2)
		return false; // default: single

	return string(argv[1]) == "multi";
}

static int32 ResolveWorkerCount(int argc, char* argv[])
{
	// server.exe multi [workerCount] [port]
	if (argc >= 3)
	{
		int32 parsed = static_cast<int32>(atoi(argv[2]));
		if (parsed > 0)
			return parsed;
	}

	uint32 hw = thread::hardware_concurrency();
	return static_cast<int32>(max(1u, hw));
}

static uint16 ResolvePort(int argc, char* argv[])
{
	// server.exe multi [workerCount] [port]
	if (argc >= 4)
	{
		int parsed = atoi(argv[3]);
		if (parsed > 0)
			return static_cast<uint16>(parsed);
	}
	return 7777;
}

string GServerAddr;

int main(int argc, char* argv[])
{
	SocketUtils::Init();
	GDBManager.Init("game.db");
	GRoomManager.Init();

	GRedisClient.Connect();
	GRedisSubscriber.Start();

	const uint16 port = ResolvePort(argc, argv);
	GServerAddr = "127.0.0.1:" + to_string(port);
	LOG_INFO("Server", "Addr=%s", GServerAddr.c_str());

	ServerServiceRef service = make_shared<ServerService>(
	NetAddress(L"127.0.0.1", port),
		make_shared<IocpCore>(),
		[]() { return make_shared<GameSession>(); },
		100); 

	assert(service->Start());
	GChatConnector.Connect(service->GetIocpCore());

	auto reportPerfIfDue = [](uint64 now, uint64& lastReportAt)
		{
			if (lastReportAt == 0)
				lastReportAt = now;

			if (now - lastReportAt < 1000)
				return;

			PacketPerfSnapshot s = ServerPacketHandler::ConsumePerfSnapshot();

			if (false)
			{
				LOG_INFO("Perf", "[1s] recv_move=%d recv_attack=%d recv_turn=%d",
					s.recvMove, s.recvAttack, s.recvTurn);
			}
			lastReportAt = now;
		};

	const bool multiMode = IsMultiMode(argc, argv);
	if (!multiMode)
	{
		LOG_INFO("Server", "Mode=single");
		uint64 lastReportAt = 0;

		while (true)
		{
			service->GetIocpCore()->Dispatch(0);
			uint64 now = GetTickCount64();
			GRoomManager.Update(now);
			reportPerfIfDue(now, lastReportAt);
			if (!GChatConnector.IsConnected())
				GChatConnector.TryReconnect(now);
			Sleep(1);
		}
	}
	else
	{
		const int32 workerCount = ResolveWorkerCount(argc, argv);
		LOG_INFO("Server", "Mode=multi, IOCP workers=%d", workerCount);

		constexpr uint32 kIocpDispatchTimeoutMs = 10;
		for (int32 i = 0; i < workerCount; ++i)
		{
			GThreadManager->Launch([service, kIocpDispatchTimeoutMs]()
				{
					while (true)
					{
						service->GetIocpCore()->Dispatch(kIocpDispatchTimeoutMs);
					}
				});
		}

		// Keep game simulation update on one thread for safe comparison.
		GThreadManager->Launch([reportPerfIfDue]()
			{
				uint64 lastReportAt = 0;

				while (true)
				{
					uint64 now = GetTickCount64();
					GRoomManager.Update(now);
					reportPerfIfDue(now, lastReportAt);
					if (!GChatConnector.IsConnected())
						GChatConnector.TryReconnect(now);
					Sleep(1);
				}
			});
	}

	GThreadManager->Join();

	SocketUtils::Clear();
}
