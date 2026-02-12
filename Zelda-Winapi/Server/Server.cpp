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

static bool IsMultiMode(int argc, char* argv[])
{
	if (argc < 2)
		return false; // default: single

	return string(argv[1]) == "multi";
}

static int32 ResolveWorkerCount(int argc, char* argv[])
{
	// server.exe multi [workerCount]
	if (argc >= 3)
	{
		int32 parsed = static_cast<int32>(atoi(argv[2]));
		if (parsed > 0)
			return parsed;
	}

	uint32 hw = thread::hardware_concurrency();
	return static_cast<int32>(max(1u, hw));
}

int main(int argc, char* argv[])
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

	auto reportPerfIfDue = [](uint64 now, uint64& lastReportAt)
		{
			if (lastReportAt == 0)
				lastReportAt = now;

			if (now - lastReportAt < 1000)
				return;

			PacketPerfSnapshot s = ServerPacketHandler::ConsumePerfSnapshot();
			cout << "[Perf][1s] recv_move=" << s.recvMove
				<< " recv_attack=" << s.recvAttack
				<< " recv_turn=" << s.recvTurn << endl;

			lastReportAt = now;
		};

	const bool multiMode = IsMultiMode(argc, argv);
	if (!multiMode)
	{
		cout << "[Server] Mode=single" << endl;
		uint64 lastReportAt = 0;

		while (true)
		{
			service->GetIocpCore()->Dispatch(0);
			uint64 now = GetTickCount64();
			GRoomManager.Update(now);
			reportPerfIfDue(now, lastReportAt);
			Sleep(1);
		}
	}
	else
	{
		const int32 workerCount = ResolveWorkerCount(argc, argv);
		cout << "[Server] Mode=multi, IOCP workers=" << workerCount << endl;

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
					Sleep(1);
				}
			});
	}

	GThreadManager->Join();

	SocketUtils::Clear();
}
