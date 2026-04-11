#include "pch.h"
#include "ChatSession.h"
#include "ChatSessionManager.h"
#include "ChatPacketHandler.h"

void ChatSession::OnConnected()
{
    cout << "[ChatServer] GameServer connected" << endl;
    GChatSessionManager.Add(GetSessionRef());
}

void ChatSession::OnDisconnected()
{
    cout << "[ChatServer] GameServer disconnected" << endl;
    GChatSessionManager.Remove(GetSessionRef());
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    ChatPacketHandler::HandlePacket(GetSessionRef(), buffer, len);
}