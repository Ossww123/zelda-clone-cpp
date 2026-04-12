#include "pch.h"
#include "ChatConnector.h"
#include "ServerPacketHandler.h"
#include "GameSessionManager.h"
#include "PartyManager.h"
#include "GameSession.h"
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

        switch (pkt.type())
        {
        case Protocol::CHAT_TYPE_GLOBAL:
            GSessionManager.Broadcast(sendBuffer);
            break;

        case Protocol::CHAT_TYPE_PARTY:
        {
            Party* party = GPartyManager.GetParty(pkt.partyid());
            if (party)
            {
                vector<uint64> memberIds(party->memberIds.begin(), party->memberIds.end());
                GSessionManager.SendToPlayerIds(memberIds, sendBuffer);
            }
            break;
        }

        case Protocol::CHAT_TYPE_WHISPER:
        {
            GameSessionRef target = GSessionManager.FindByPlayerName(pkt.target());
            if (target)
            {
                target->Send(sendBuffer);
            }
            else
            {
                // 대상이 없으면 sender에게 에러 메시지
                Protocol::S_Chat errPkt;
                errPkt.set_sender("System");
                errPkt.set_type(Protocol::CHAT_TYPE_WHISPER);
                errPkt.set_msg("[System] 대상 플레이어를 찾을 수 없습니다.");
                SendBufferRef errBuffer = ServerPacketHandler::Make_S_Chat(errPkt);
                GameSessionRef senderSession = GSessionManager.FindByPlayerName(pkt.sender());
                if (senderSession)
                    senderSession->Send(errBuffer);
            }
            break;
        }

        default:
            break;
        }
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