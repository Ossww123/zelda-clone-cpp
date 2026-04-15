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
        cout << "[ChatServer] Unknown packet id: " << header->id << endl;
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

    cout << "[ChatServer] RelayChat from=" << pkt.sender()
        << " server=:" << fromServer
        << " type=" << pkt.type()
        << " msg=" << pkt.msg() << endl;

    Protocol::SS_BroadcastChat broadcast;
    broadcast.set_sender(pkt.sender());
    broadcast.set_type(pkt.type());
    broadcast.set_msg(pkt.msg());
    broadcast.set_target(pkt.target());
    broadcast.set_partyid(pkt.partyid());

    SendBufferRef sendBuffer = Make_SS_BroadcastChat(broadcast);
    // GChatSessionManager.BroadcastAll(sendBuffer);

    std::string payload;
    broadcast.SerializeToString(&payload);

    if (!GRedisClient.IsConnected())
    {
        GChatSessionManager.BroadcastAll(sendBuffer);
        return;
    }

    if (pkt.type() == Protocol::CHAT_TYPE_GLOBAL)
    {
        cout << "[ChatServer] PUBLISH chat:global" << endl;
        GRedisClient.Get().publish("chat:global", payload);
    }
    else if (pkt.type() == Protocol::CHAT_TYPE_WHISPER) {
        auto val = GRedisClient.Get().get("player:loc:" + pkt.target());
        if (val)
        {
            // 대상 서버 채널로 PUBLISH
            string channel = "chat:whisper:" + *val;
            cout << "[ChatServer] PUBLISH " << channel << " (to=" << pkt.target() << ")" << endl;
            GRedisClient.Get().publish(channel, payload);
        }
        else
        {
            // 대상 없음 → sender 서버에 에러 메시지 PUBLISH
            Protocol::SS_BroadcastChat errPkt;
            errPkt.set_sender("System");
            errPkt.set_type(Protocol::CHAT_TYPE_WHISPER);
            errPkt.set_msg("[System] 대상 플레이어를 찾을 수 없습니다.");
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