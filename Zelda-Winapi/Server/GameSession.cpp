#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
#include "GameRoomManager.h"
#include "PartyManager.h"
#include "Player.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(GetSessionRef());

	// �α��� ��Ŷ

	Send(ServerPacketHandler::Make_S_EnterGame());

	// ���� ����
	GameRoomRef room = GRoomManager.GetStaticRoom(FieldId::Town, 1);
	if (room)
	{
		GameSessionRef self = GetSessionRef();
		room->PushJob([room, self]()
			{
				room->EnterRoom(self);
			});
	}
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(GetSessionRef());

	// 파티 정리
	PlayerRef p = dynamic_pointer_cast<Player>(player.lock());
	if (p)
	{
		uint64 playerId = p->info.objectid();
		uint64 partyId = GPartyManager.GetPartyIdByPlayer(playerId);
		if (partyId != 0)
		{
			GPartyManager.RemoveMember(partyId, playerId);

			// 남은 파티원에게 알림
			Party* remaining = GPartyManager.GetParty(partyId);
			GameRoomRef room = gameRoom.lock();
			if (remaining && room)
			{
				for (uint64 memberId : remaining->memberIds)
				{
					GameObjectRef obj = room->FindObject(memberId);
					if (!obj) continue;
					PlayerRef mp = dynamic_pointer_cast<Player>(obj);
					if (!mp || !mp->session) continue;

					// 파티가 유지되면 업데이트, 1명이면 해산 처리는 RemoveMember에서 이미 수행
					Protocol::S_PartyUpdate updatePkt;
					for (uint64 mid : remaining->memberIds)
					{
						Protocol::PartyMemberInfo* m = updatePkt.add_members();
						m->set_playerid(mid);
						m->set_isleader(mid == remaining->leaderId);
					}
					SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(updatePkt, S_PartyUpdate);
					mp->session->Send(sendBuffer);
				}
			}
			else if (!remaining && room)
			{
				// 파티 해산됨 - 남은 1명에게 S_PartyLeave (RemoveMember→DisbandParty가 이미 처리)
				// DisbandParty에서 _playerToParty를 지우므로 여기선 별도 처리 불필요
			}
		}
	}

	// 방 퇴장
	GameRoomRef room = gameRoom.lock();
	if (room)
	{
		GameSessionRef self = GetSessionRef();
		room->PushJob([room, self]()
			{
				room->LeaveRoom(self);
			});
	}
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerPacketHandler::HandlePacket(static_pointer_cast<GameSession>(shared_from_this()), buffer, len);
}


void GameSession::OnSend(int32 len)
{
	// cout << "OnSend Len = " << len << endl;
}