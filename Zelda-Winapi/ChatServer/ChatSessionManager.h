#pragma once
class ChatSessionManager
{
public:
    void Add(ChatSessionRef session);
    void Remove(ChatSessionRef session);
    void BroadcastAll(SendBufferRef sendBuffer);

private:
    USE_LOCK;
    set<ChatSessionRef> _sessions;
};

extern ChatSessionManager GChatSessionManager;