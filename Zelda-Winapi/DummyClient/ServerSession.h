#pragma once

class ServerSession : public PacketSession
{
public:
    ServerSession(int32 botId, int32 moveIntervalMs, int32 attackIntervalMs);

    virtual void OnConnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override {}
    virtual void OnDisconnected() override {}

    void Tick(const chrono::steady_clock::time_point& now);

private:
    int32 _botId;                                       // 봇 고유 ID (bot_1, bot_2 ... )
    int32 _moveIntervalMs;                              // 이동 패킷 전송 간격 (ms)
    int32 _attackIntervalMs;                            // 공격 패킷 전송 간격 (ms)

    atomic<bool> _loggedIn{ false };                    // S_EnterGame 수신 후 true - IOCP 스레드가 쓰고 메인스레드가 읽으므로 atomic

    mt19937 _rng;                                       // 난수 생성기 - 이동/공격 방향을 랜덤으로 뽑기 위해
    uniform_int_distribution<int32> _dirDist{ 0, 3 };   // 방향 0 ~ 3

    chrono::steady_clock::time_point _nextMoveAt;       // 다음 이동 패킷 보낼 시각
    chrono::steady_clock::time_point _nextAttackAt;     // 다음 공격 패킷 보낼 시각
};
