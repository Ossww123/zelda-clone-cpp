#include "pch.h"
#include <iostream>
#include <random>
#include <atomic>
#include <chrono>
#include <algorithm>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "ClientPacketHandler.h"

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

class ServerSession : public PacketSession
{
public:
    explicit ServerSession(int32 botId) : _botId(botId)
    {
        _rng.seed(static_cast<uint32>(GetTickCount64()) + static_cast<uint32>(_botId));
    }

    ~ServerSession()
    {
        StopBotLoop();
    }

    virtual void OnConnected() override
    {
        _running.store(true);

        string username = "bot_" + to_string(_botId);
        SendBufferRef login = ClientPacketHandler::Make_C_Login(username);
        Send(login);

        _nextMoveAt = chrono::steady_clock::now();
        _nextAttackAt = chrono::steady_clock::now();

        _botThread = thread([this]() { BotLoop(); });
    }

    virtual void OnRecvPacket(BYTE* buffer, int32 len) override
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
        if (header->id == S_EnterGame)
        {
            Protocol::S_EnterGame pkt;
            if (pkt.ParseFromArray(&header[1], header->size - sizeof(PacketHeader)) && pkt.success())
                _loggedIn.store(true);
        }

        ClientPacketHandler::HandlePacket(buffer, len);
    }

    virtual void OnSend(int32 len) override
    {
    }

    virtual void OnDisconnected() override
    {
        StopBotLoop();
    }

private:
    void BotLoop()
    {
        while (_running.load())
        {
            if (!_loggedIn.load())
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;
            }

            const auto now = chrono::steady_clock::now();

            if (now >= _nextMoveAt)
            {
                Protocol::DIR_TYPE dir = static_cast<Protocol::DIR_TYPE>(_dirDist(_rng));
                SendBufferRef movePkt = ClientPacketHandler::Make_C_Move(dir);
                Send(movePkt);
                _nextMoveAt = now + chrono::milliseconds(GMoveIntervalMs);
            }

            if (now >= _nextAttackAt)
            {
                Protocol::DIR_TYPE dir = static_cast<Protocol::DIR_TYPE>(_dirDist(_rng));
                SendBufferRef atkPkt = ClientPacketHandler::Make_C_Attack(dir, Protocol::WEAPON_TYPE_SWORD);
                Send(atkPkt);
                _nextAttackAt = now + chrono::milliseconds(GAttackIntervalMs);
            }

            this_thread::sleep_for(chrono::milliseconds(1));
        }
    }

    void StopBotLoop()
    {
        _running.store(false);

        if (_botThread.joinable())
            _botThread.join();
    }

private:
    int32 _botId = 0;
    atomic<bool> _running{ false };
    atomic<bool> _loggedIn{ false };
    thread _botThread;
    mt19937 _rng;
    uniform_int_distribution<int32> _dirDist{ 0, 3 };
    chrono::steady_clock::time_point _nextMoveAt;
    chrono::steady_clock::time_point _nextAttackAt;
};

int main(int argc, char* argv[])
{
    ParseArgs(argc, argv);

    cout << "[DummyClient] bots=" << GBotCount
        << " iocpThreads=" << GIocpThreads
        << " moveMs=" << GMoveIntervalMs
        << " attackMs=" << GAttackIntervalMs << endl;

    this_thread::sleep_for(chrono::seconds(1));

    SocketUtils::Init();

    atomic<int32> botIdGen = 1;
    ClientServiceRef service = make_shared<ClientService>(
        NetAddress(L"127.0.0.1", 7777),
        make_shared<IocpCore>(),
        [&botIdGen]() { return make_shared<ServerSession>(botIdGen.fetch_add(1)); },
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

    while (true)
    {
        this_thread::sleep_for(chrono::seconds(1));

        DummyClientStats s = ClientPacketHandler::ConsumeStats();
        cout << "[DummyClient][1s]"
            << " S_EnterGame=" << s.recvEnterGame
            << " S_Move=" << s.recvMove
            << " S_Attack=" << s.recvAttack
            << " S_Damaged=" << s.recvDamaged
            << " S_Turn=" << s.recvTurn
            << endl;
    }
}
