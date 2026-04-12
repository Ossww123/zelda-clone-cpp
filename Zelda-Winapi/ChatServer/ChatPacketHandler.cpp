#include "pch.h"
#include "ChatPacketHandler.h"
#include "ChatSessionManager.h"

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

    cout << "[ChatServer] RelayChat from=" << pkt.sender()
        << " type=" << pkt.type()
        << " msg=" << pkt.msg() << endl;

    Protocol::SS_BroadcastChat broadcast;
    broadcast.set_sender(pkt.sender());
    broadcast.set_type(pkt.type());
    broadcast.set_msg(pkt.msg());
    broadcast.set_target(pkt.target());
    broadcast.set_partyid(pkt.partyid());

    SendBufferRef sendBuffer = Make_SS_BroadcastChat(broadcast);
    GChatSessionManager.BroadcastAll(sendBuffer);
}

SendBufferRef ChatPacketHandler::Make_SS_BroadcastChat(const Protocol::SS_BroadcastChat& pkt)
{
    return MakeSendBuffer(pkt, SS_BroadcastChat);
}