#pragma once
#include "Session.h"

enum
{
    SS_RelayChat = 301,
    SS_BroadcastChat = 302,
};

// GameServer ������ ChatServer�� �����ϴ� ����
class ChatRelaySession : public PacketSession
{
public:
    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override {}
};

// ChatServer ���� ���� �̱���
class ChatConnector
{
public:
    void Connect(IocpCoreRef iocpCore);
    void TryReconnect(uint64 now);
    void Send(SendBufferRef sendBuffer);
    bool IsConnected() const { return _session && _session->IsConnected(); }

private:
    IocpCoreRef _iocpCore;
    ClientServiceRef _service;
    shared_ptr<ChatRelaySession> _session;
    uint64 _lastTryAt = 0;
    static constexpr uint64 kReconnectIntervalMs = 5000;
};

extern ChatConnector GChatConnector;