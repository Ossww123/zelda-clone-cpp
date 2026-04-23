#include "pch.h"
#include "ChatPacketHandler.h"
#include "ChatSessionManager.h"
#include "ChatSession.h"
#include "RedisClient.h"

enum
{
    SS_RelayChat = 301,
    SS_BroadcastChat = 302,
};

void ChatPacketHandler::HandlePacket(ChatSessionRef session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    switch (header->id)
    {
    case SS_RelayChat:
        Handle_SS_RelayChat(session, buffer, len);
        break;
    default:
        LOG_WARN("Packet", "Unknown packet id: %d", header->id);
        break;
    }
}

void ChatPacketHandler::Handle_SS_RelayChat(ChatSessionRef session, BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    Protocol::SS_RelayChat pkt;
    pkt.ParseFromArray(&header[1], len - sizeof(PacketHeader));

    NetAddress fromAddr = session->GetAddress();
    string fromServer = to_string(fromAddr.GetPort());

    LOG_INFO("Relay", "RelayChat from=%s server=:%s type=%d msg=%s",
        pkt.sender().c_str(), fromServer.c_str(), pkt.type(), pkt.msg().c_str());

    Protocol::SS_BroadcastChat broadcast;
    broadcast.set_sender(pkt.sender());
    broadcast.set_type(pkt.type());
    broadcast.set_msg(pkt.msg());
    broadcast.set_target(pkt.target());
    broadcast.set_partyid(pkt.partyid());

    SendBufferRef sendBuffer = Make_SS_BroadcastChat(broadcast);

    std::string payload;
    broadcast.SerializeToString(&payload);

    if (!GRedisClient.IsConnected())
    {
        GChatSessionManager.BroadcastAll(sendBuffer);
        return;
    }

    if (pkt.type() == Protocol::CHAT_TYPE_GLOBAL)
    {
        LOG_CHAT(pkt.sender(), "GLOBAL", "", pkt.msg());
        GRedisClient.Get().publish("chat:global", payload);
    }
    else if (pkt.type() == Protocol::CHAT_TYPE_WHISPER) {
        auto val = GRedisClient.Get().get("player:loc:" + pkt.target());
        if (val)
        {
            string channel = "chat:whisper:" + *val;
            LOG_CHAT(pkt.sender(), "WHISPER", pkt.target(), pkt.msg());
            GRedisClient.Get().publish(channel, payload);
        }
        else
        {
            Protocol::SS_BroadcastChat errPkt;
            errPkt.set_sender("System");
            errPkt.set_type(Protocol::CHAT_TYPE_WHISPER);
            errPkt.set_msg("[System] 해당 플레이어를 찾을 수 없습니다.");
            errPkt.set_target(pkt.sender());

            string errPayload;
            errPkt.SerializeToString(&errPayload);

            auto senderLoc = GRedisClient.Get().get("player:loc:" + pkt.sender());
            if (senderLoc)
                GRedisClient.Get().publish("chat:whisper:" + *senderLoc, errPayload);
        }
    }
    else
    {
        GChatSessionManager.BroadcastAll(sendBuffer);
    }
}

SendBufferRef ChatPacketHandler::Make_SS_BroadcastChat(const Protocol::SS_BroadcastChat& pkt)
{
    return MakeSendBuffer(pkt, SS_BroadcastChat);
}