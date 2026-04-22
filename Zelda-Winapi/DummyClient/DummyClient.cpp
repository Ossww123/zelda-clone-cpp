#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "ClientPacketHandler.h"
#include "ServerSession.h"

using namespace std;

static int32 GBotCount = 100;
static int32 GIocpThreads = 4;
static int32 GMoveIntervalMs = 120;
static int32 GAttackIntervalMs = 700;

static void ParseArgs(int argc, char* argv[])
{
    if (argc >= 2)
        GBotCount = (std::max)(1, atoi(argv[1]));
    if (argc >= 3)
        GIocpThreads = (std::max)(1, atoi(argv[2]));
    if (argc >= 4)
        GMoveIntervalMs = (std::max)(10, atoi(argv[3]));
    if (argc >= 5)
        GAttackIntervalMs = (std::max)(50, atoi(argv[4]));
}

int main(int argc, char* argv[])
{
    ParseArgs(argc, argv);

    cout << "[DummyClient] bots=" << GBotCount
        << " iocpThreads=" << GIocpThreads
        << " moveMs=" << GMoveIntervalMs
        << " attackMs=" << GAttackIntervalMs << endl;

    this_thread::sleep_for(chrono::seconds(1)); // 서버 기동 대기

    SocketUtils::Init();

    vector<shared_ptr<ServerSession>> bots;
    bots.reserve(GBotCount);

    atomic<int32> botIdGen = 1;
    ClientServiceRef service = make_shared<ClientService>(
        NetAddress(L"127.0.0.1", 7777),
        make_shared<IocpCore>(),
        [&botIdGen, &bots]() {
            auto session = make_shared<ServerSession>(botIdGen.fetch_add(1), GMoveIntervalMs, GAttackIntervalMs);
            bots.push_back(session);
            return session;
        },
        GBotCount);

    assert(service->Start());

    for (int32 i = 0; i < GIocpThreads; i++)
    {
        GThreadManager->Launch([service]()
            {
                while (true)
                    service->GetIocpCore()->Dispatch();
            });
    }

    auto lastStatTime = chrono::steady_clock::now();

    while (true)
    {
        const auto now = chrono::steady_clock::now();

        for (auto& bot : bots)
            bot->Tick(now);

        if (now - lastStatTime >= chrono::seconds(1))
        {
            DummyClientStats s = ClientPacketHandler::ConsumeStats();
            cout << "[DummyClient][1s]"
                << " S_EnterGame=" << s.recvEnterGame
                << " S_Move=" << s.recvMove
                << " S_Attack=" << s.recvAttack
                << " S_Damaged=" << s.recvDamaged
                << " S_Turn=" << s.recvTurn
                << endl;
            lastStatTime = now;
        }

        this_thread::sleep_for(chrono::milliseconds(1));
    }
}
