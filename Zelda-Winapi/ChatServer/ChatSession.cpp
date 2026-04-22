#include "pch.h"
#include "ChatSession.h"
#include "ChatSessionManager.h"
#include "ChatPacketHandler.h"

void ChatSession::OnConnected()
{
    LOG_INFO("Session", "GameServer connected");
    GChatSessionManager.Add(GetSessionRef());
}

void ChatSession::OnDisconnected()
{
    LOG_INFO("Session", "GameServer disconnected");
    GChatSessionManager.Remove(GetSessionRef());
}

void ChatSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    ChatPacketHandler::HandlePacket(GetSessionRef(), buffer, len);
}