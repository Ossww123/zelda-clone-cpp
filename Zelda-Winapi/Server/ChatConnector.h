#pragma once
#include "Session.h"

enum
{
    SS_RelayChat = 301,
    SS_BroadcastChat = 302,
};

// GameServer 측에서 ChatServer로 연결하는 세션
class ChatRelaySession : public PacketSession
{
public:
    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override {}
};

// ChatServer 연결 관리 싱글톤
class ChatConnector
{
public:
    void Connect(IocpCoreRef iocpCore);
    void Send(SendBufferRef sendBuffer);
    bool IsConnected() const { return _session && _session->IsConnected(); }

private:
    ClientServiceRef _service;
    shared_ptr<ChatRelaySession> _session;
};

extern ChatConnector GChatConnector;