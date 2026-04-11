#include "pch.h"
#include "ChatConnector.h"
#include "ServerPacketHandler.h"
#include "GameSessionManager.h"
#include "Service.h"

ChatConnector GChatConnector;

void ChatRelaySession::OnConnected()
{
    cout << "[Server] Connected to ChatServer" << endl;
}

void ChatRelaySession::OnDisconnected()
{
    cout << "[Server] Disconnected from ChatServer" << endl;
}

void ChatRelaySession::OnRecvPacket(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    if (header->id == SS_BroadcastChat)
    {
        Protocol::SS_BroadcastChat pkt;
        pkt.ParseFromArray(&header[1], len - sizeof(PacketHeader));

        Protocol::S_Chat chatPkt;
        chatPkt.set_sender(pkt.sender());
        chatPkt.set_type(pkt.type());
        chatPkt.set_msg(pkt.msg());

        SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Chat(chatPkt);
        GSessionManager.Broadcast(sendBuffer);
    }
}

void ChatConnector::Connect(IocpCoreRef iocpCore)
{
    _session = make_shared<ChatRelaySession>();

    _service = make_shared<ClientService>(
        NetAddress(L"127.0.0.1", 8888),
        iocpCore,
        [this]() { return _session; },
        1
    );

    if (!_service->Start())
    {
        cout << "[Server] Failed to connect to ChatServer" << endl;
    }
}

void ChatConnector::Send(SendBufferRef sendBuffer)
{
    if (!IsConnected())
    {
        cout << "[Server] ChatServer not connected, dropping packet" << endl;
        return;
    }

    _session->Send(sendBuffer);
}