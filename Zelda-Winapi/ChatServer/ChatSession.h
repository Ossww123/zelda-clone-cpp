#pragma once                                          t
#include "Session.h"

class ChatSession : public PacketSession
{
public:
    ~ChatSession()
    {
        LOG_INFO("ChatSession", "~ChatSession");
    }

    virtual void OnConnected() override;
    virtual void OnDisconnected() override;
    virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
    virtual void OnSend(int32 len) override {}

    ChatSessionRef GetSessionRef() { return static_pointer_cast<ChatSession>(shared_from_this()); }
};