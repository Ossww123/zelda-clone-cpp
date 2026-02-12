#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameRoom.h"
#include "GameRoomManager.h"
#include "Player.h"
#include "PartyManager.h"
#include "DBManager.h"


void ServerPacketHandler::HandlePacket(GameSessionRef session, BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	switch (header.id)
	{
	case C_Move:
		Handle_C_Move(session, buffer, len);
		break;
	case C_Attack:
		Handle_C_Attack(session, buffer, len);
		break;
	case C_ChangeMap:
		Handle_C_ChangeMap(session, buffer, len);
		break;
	case C_Turn:
		Handle_C_Turn(session, buffer, len);
		break;
	case C_EquipItem:
		Handle_C_EquipItem(session, buffer, len);
		break;
	case C_UnequipItem:
		Handle_C_UnequipItem(session, buffer, len);
		break;
	case C_UseItem:
		Handle_C_UseItem(session, buffer, len);
		break;
	case C_PartyInvite:
		Handle_C_PartyInvite(session, buffer, len);
		break;

	case C_PartyAnswer:
		Handle_C_PartyAnswer(session, buffer, len);
		break;
	case C_PartyLeave:
		Handle_C_PartyLeave(session, buffer, len);
		break;
	case C_Login:
		Handle_C_Login(session, buffer, len);
		break;
	// [AUTO-GEN SWITCH BEGIN]

	// [AUTO-GEN SWITCH END]
	default:
		break;
	}
}

void ServerPacketHandler::Handle_C_Move(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::C_Move pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	//
	GameRoomRef gameRoom = session->gameRoom.lock();
	if (gameRoom == nullptr)
		return;

	gameRoom->PushJob([gameRoom, session, pkt]()
		{
			gameRoom->Handle_C_Move(session, pkt);
		});
}

void ServerPacketHandler::Handle_C_Attack(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::C_Attack pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	//
	GameRoomRef gameRoom = session->gameRoom.lock();
	if (gameRoom == nullptr)
		return;

	gameRoom->PushJob([gameRoom, session, pkt]()
		{
			gameRoom->Handle_C_Attack(session, pkt);
		});
}

void ServerPacketHandler::Handle_C_ChangeMap(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_ChangeMap pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	GameRoomRef fromRoom = session->gameRoom.lock();
	if (!fromRoom)
		return;

	fromRoom->PushJob([session, pkt, fromRoom]()
		{
			GameRoomRef from = session->gameRoom.lock();
			if (!from)
				return;

			PlayerRef requester = dynamic_pointer_cast<Player>(session->player.lock());
			if (!requester)
				return;

			uint64 requesterId = requester->info.objectid();

			GameRoomRef to = nullptr;
			uint64 instanceId = 0;

			if (pkt.mapid() == Protocol::MAP_ID_TOWN)
			{
				to = GRoomManager.GetStaticRoom(FieldId::Town, pkt.channel());
			}
			else if (pkt.mapid() == Protocol::MAP_ID_DUNGEON)
			{
				// 파티원(비리더)이면 던전 진입 불가
				if (GPartyManager.IsInParty(requesterId) && !GPartyManager.IsLeader(requesterId))
				{
					Protocol::S_ChangeMap sendPkt;
					sendPkt.set_success(false);
					sendPkt.set_mapid(pkt.mapid());
					sendPkt.set_channel(pkt.channel());
					sendPkt.set_instanceid(0);
					session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
					return;
				}

				instanceId = GRoomManager.CreateDungeonInstance();
				to = GRoomManager.GetDungeonInstance(instanceId);
			}

			if (!to)
			{
				Protocol::S_ChangeMap sendPkt;
				sendPkt.set_success(false);
				sendPkt.set_mapid(pkt.mapid());
				sendPkt.set_channel(pkt.channel());
				sendPkt.set_instanceid(0);

				session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
				return;
			}

			// 파티장이 던전 진입 시 → 같은 방의 파티원 전원 이동
			if (pkt.mapid() == Protocol::MAP_ID_DUNGEON && GPartyManager.IsLeader(requesterId))
			{
				uint64 partyId = GPartyManager.GetPartyIdByPlayer(requesterId);
				Party* party = GPartyManager.GetParty(partyId);
				if (party)
				{
					vector<pair<GameSessionRef, PlayerRef>> toMove;
					for (uint64 memberId : party->memberIds)
					{
						GameObjectRef obj = from->FindObject(memberId);
						if (!obj)
							continue;
						PlayerRef p = dynamic_pointer_cast<Player>(obj);
						if (p && p->session)
							toMove.push_back({ p->session, p });
					}

					for (auto& [s, p] : toMove)
					{
						Protocol::S_ChangeMap sendPkt;
						sendPkt.set_success(true);
						sendPkt.set_mapid(pkt.mapid());
						sendPkt.set_channel(pkt.channel());
						sendPkt.set_instanceid(instanceId);
						s->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));

						from->LeaveRoom(s);
						to->PushJob([to, s, p]()
							{
								to->EnterRoom(s, p);
							});
					}
					return;
				}
			}

			// 솔로 이동 (기존 로직) — 기존 플레이어 유지
			{
				Protocol::S_ChangeMap sendPkt;
				sendPkt.set_success(true);
				sendPkt.set_mapid(pkt.mapid());
				sendPkt.set_channel(pkt.channel());
				sendPkt.set_instanceid(instanceId);

				session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
			}

			// LeaveRoom 전에 플레이어 강한 참조 확보
			PlayerRef movingPlayer = dynamic_pointer_cast<Player>(session->player.lock());
			from->LeaveRoom(session);

			to->PushJob([to, session, movingPlayer]()
				{
					to->EnterRoom(session, movingPlayer);
				});
		});
}

void ServerPacketHandler::Handle_C_Turn(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::C_Turn pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	//
	GameRoomRef gameRoom = session->gameRoom.lock();
	if (gameRoom == nullptr)
		return;

	gameRoom->PushJob([gameRoom, session, pkt]()
		{
			gameRoom->Handle_C_Turn(session, pkt);
		});
}


SendBufferRef ServerPacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs)
{
	Protocol::S_TEST pkt;

	pkt.set_id(10);
	pkt.set_hp(100);
	pkt.set_attack(10);

	{
		Protocol::BuffData* data = pkt.add_buffs();
		data->set_buffid(100);
		data->set_remaintime(1.2f);
		{
			data->add_victims(10);
		}
	}
	{
		Protocol::BuffData* data = pkt.add_buffs();
		data->set_buffid(200);
		data->set_remaintime(2.2f);
		{
			data->add_victims(20);
		}
	}

	return MakeSendBuffer(pkt, S_TEST);
}


SendBufferRef ServerPacketHandler::Make_S_EnterGame()
{
	Protocol::S_EnterGame pkt;

	pkt.set_success(true);
	pkt.set_accountid(0); // DB

	return MakeSendBuffer(pkt, S_EnterGame);
}

SendBufferRef ServerPacketHandler::Make_S_MyPlayer(const Protocol::ObjectInfo info)
{
	Protocol::S_MyPlayer pkt;

	Protocol::ObjectInfo* objectInfo = pkt.mutable_info();
	*objectInfo = info;

	return MakeSendBuffer(pkt, S_MyPlayer);
}

SendBufferRef ServerPacketHandler::Make_S_AddObject(const Protocol::S_AddObject& pkt)
{
	return MakeSendBuffer(pkt, S_AddObject);
}

SendBufferRef ServerPacketHandler::Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt)
{
	return MakeSendBuffer(pkt, S_RemoveObject);
}

SendBufferRef ServerPacketHandler::Make_S_Move(const Protocol::ObjectInfo& info)
{
	Protocol::S_Move pkt;

	Protocol::ObjectInfo* objectInfo = pkt.mutable_info();
	*objectInfo = info;

	return MakeSendBuffer(pkt, S_Move);
}

SendBufferRef ServerPacketHandler::Make_S_Attack(const Protocol::S_Attack& pkt)
{
	return MakeSendBuffer(pkt, S_Attack);
}

SendBufferRef ServerPacketHandler::Make_S_Damaged(const Protocol::S_Damaged& pkt)
{
	return MakeSendBuffer(pkt, S_Damaged);
}

SendBufferRef ServerPacketHandler::Make_S_ChangeMap(const Protocol::S_ChangeMap& pkt)
{
	return MakeSendBuffer(pkt, S_ChangeMap);
}

SendBufferRef ServerPacketHandler::Make_S_GainExp(const Protocol::S_GainExp& pkt)
{
	return MakeSendBuffer(pkt, S_GainExp);
}

SendBufferRef ServerPacketHandler::Make_S_LevelUp(const Protocol::S_LevelUp& pkt)
{
	return MakeSendBuffer(pkt, S_LevelUp);
}

SendBufferRef ServerPacketHandler::Make_S_Turn(const Protocol::S_Turn& pkt)
{
	return MakeSendBuffer(pkt, S_Turn);
}

void ServerPacketHandler::Handle_C_EquipItem(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_EquipItem pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	GameRoomRef gameRoom = session->gameRoom.lock();
	if (gameRoom == nullptr)
		return;

	gameRoom->PushJob([session, pkt]()
		{
			PlayerRef player = session->player.lock();
			if (player)
				player->EquipItem(pkt.slot());
		});
}

void ServerPacketHandler::Handle_C_UnequipItem(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_UnequipItem pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	GameRoomRef gameRoom = session->gameRoom.lock();
	if (gameRoom == nullptr)
		return;

	gameRoom->PushJob([session, pkt]()
		{
			PlayerRef player = session->player.lock();
			if (player)
				player->UnequipItem(pkt.equiptype());
		});
}

void ServerPacketHandler::Handle_C_UseItem(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_UseItem pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	GameRoomRef gameRoom = session->gameRoom.lock();
	if (gameRoom == nullptr)
		return;

	gameRoom->PushJob([session, pkt]()
		{
			PlayerRef player = session->player.lock();
			if (player)
				player->UseItem(pkt.slot());
		});
}

SendBufferRef ServerPacketHandler::Make_S_InventoryData(const vector<Protocol::ItemInfo>& items, const Protocol::ItemInfo& equippedWeapon, const Protocol::ItemInfo& equippedArmor, const Protocol::ItemInfo& equippedPotion)
{
	Protocol::S_InventoryData pkt;
	for (const auto& item : items)
		*pkt.add_items() = item;
	*pkt.mutable_equippedweapon() = equippedWeapon;
	*pkt.mutable_equippedarmor() = equippedArmor;
	*pkt.mutable_equippedpotion() = equippedPotion;
	return MakeSendBuffer(pkt, S_InventoryData);
}

SendBufferRef ServerPacketHandler::Make_S_AddItem(int32 itemId, int32 slot, int32 count)
{
	Protocol::S_AddItem pkt;
	pkt.set_itemid(itemId);
	pkt.set_slot(slot);
	pkt.set_count(count);
	return MakeSendBuffer(pkt, S_AddItem);
}

SendBufferRef ServerPacketHandler::Make_S_EquipItem(int32 equipType, int32 storageSlot, int32 storageItemId, int32 storageItemCount, int32 equipItemId, int32 equipItemCount, int32 attack, int32 defence)
{
	Protocol::S_EquipItem pkt;
	pkt.set_equiptype(equipType);
	pkt.set_storageslot(storageSlot);
	pkt.set_storageitemid(storageItemId);
	pkt.set_storageitemcount(storageItemCount);
	pkt.set_equipitemid(equipItemId);
	pkt.set_equipitemcount(equipItemCount);
	pkt.set_attack(attack);
	pkt.set_defence(defence);
	return MakeSendBuffer(pkt, S_EquipItem);
}

SendBufferRef ServerPacketHandler::Make_S_UnequipItem(int32 equipType, int32 storageSlot, int32 attack, int32 defence)
{
	Protocol::S_UnequipItem pkt;
	pkt.set_equiptype(equipType);
	pkt.set_storageslot(storageSlot);
	pkt.set_attack(attack);
	pkt.set_defence(defence);
	return MakeSendBuffer(pkt, S_UnequipItem);
}

SendBufferRef ServerPacketHandler::Make_S_UseItem(int32 equipType, int32 remainCount, int32 newHp)
{
	Protocol::S_UseItem pkt;
	pkt.set_equiptype(equipType);
	pkt.set_remaincount(remainCount);
	pkt.set_newhp(newHp);
	return MakeSendBuffer(pkt, S_UseItem);
}

// ---- Party ----

static void SendPartyUpdateFromRoom(GameRoomRef room, uint64 partyId)
{
	Party* party = GPartyManager.GetParty(partyId);
	if (!party)
		return;

	Protocol::S_PartyUpdate pkt;
	for (uint64 memberId : party->memberIds)
	{
		Protocol::PartyMemberInfo* m = pkt.add_members();
		m->set_playerid(memberId);
		m->set_isleader(memberId == party->leaderId);

		GameObjectRef obj = room->FindObject(memberId);
		if (obj)
		{
			PlayerRef p = dynamic_pointer_cast<Player>(obj);
			if (p)
			{
				m->set_name(p->info.name());
				m->set_level(p->GetLevel());
				m->set_hp(p->info.hp());
				m->set_maxhp(p->info.maxhp());
			}
		}
		else
		{
			auto nameIt = party->memberNames.find(memberId);
			string name = (nameIt != party->memberNames.end()) ? nameIt->second : "Unknown";
			m->set_name(name + "(Away)");
		}
	}

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt, S_PartyUpdate);

	for (uint64 memberId : party->memberIds)
	{
		GameObjectRef obj = room->FindObject(memberId);
		if (!obj)
			continue;
		PlayerRef player = dynamic_pointer_cast<Player>(obj);
		if (player && player->session)
			player->session->Send(sendBuffer);
	}
}

void ServerPacketHandler::Handle_C_PartyInvite(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_PartyInvite pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	GameRoomRef gameRoom = session->gameRoom.lock();
	if (!gameRoom)
		return;

	gameRoom->PushJob([session, pkt, gameRoom]()
		{
			PlayerRef inviter = dynamic_pointer_cast<Player>(session->player.lock());
			if (!inviter)
				return;

			uint64 inviterId = inviter->info.objectid();
			uint64 targetId = pkt.targetid();

			// 자기 자신 초대 불가
			if (inviterId == targetId)
				return;

			// 이미 파티에 있으면 리더만 초대 가능
			if (GPartyManager.IsInParty(inviterId) && !GPartyManager.IsLeader(inviterId))
				return;

			// 대상이 이미 파티에 있으면 거절
			if (GPartyManager.IsInParty(targetId))
				return;

			// 파티 인원 확인
			if (GPartyManager.IsInParty(inviterId))
			{
				Party* party = GPartyManager.GetParty(GPartyManager.GetPartyIdByPlayer(inviterId));
				if (party && (int32)party->memberIds.size() >= PartyManager::MAX_PARTY_SIZE)
					return;
			}

			// 같은 방에 있는지 확인
			GameObjectRef targetObj = gameRoom->FindObject(targetId);
			if (!targetObj)
				return;
			PlayerRef target = dynamic_pointer_cast<Player>(targetObj);
			if (!target || !target->session)
				return;

			// 대상에게 초대 전송
			Protocol::S_PartyInvite sendPkt;
			sendPkt.set_inviterid(inviterId);
			sendPkt.set_invitername(inviter->info.name());
			SendBufferRef sendBuffer = MakeSendBuffer(sendPkt, S_PartyInvite);
			target->session->Send(sendBuffer);

			cout << "[Party] " << inviter->info.name() << " invited player " << targetId << endl;
		});
}

void ServerPacketHandler::Handle_C_PartyAnswer(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_PartyAnswer pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	GameRoomRef gameRoom = session->gameRoom.lock();
	if (!gameRoom)
		return;

	gameRoom->PushJob([session, pkt, gameRoom]()
		{
			PlayerRef acceptor = dynamic_pointer_cast<Player>(session->player.lock());
			if (!acceptor)
				return;

			uint64 acceptorId = acceptor->info.objectid();
			uint64 inviterId = pkt.inviterid();
			bool accept = pkt.accept();

			if (!accept)
				return;

			// 수락자가 이미 파티에 있으면 거절
			if (GPartyManager.IsInParty(acceptorId))
				return;

			uint64 partyId = GPartyManager.GetPartyIdByPlayer(inviterId);
			if (partyId == 0)
			{
				// 초대자가 아직 파티가 없으면 새로 생성
				partyId = GPartyManager.CreateParty(inviterId);
				if (partyId == 0)
					return;

				// 초대자 이름 저장
				GameObjectRef inviterObj = gameRoom->FindObject(inviterId);
				if (inviterObj)
				{
					Party* p = GPartyManager.GetParty(partyId);
					if (p) p->memberNames[inviterId] = inviterObj->info.name();
				}
			}

			if (!GPartyManager.AddMember(partyId, acceptorId))
				return;

			// 수락자 이름 저장
			{
				Party* p = GPartyManager.GetParty(partyId);
				if (p) p->memberNames[acceptorId] = acceptor->info.name();
			}

			// 전체 파티원에게 업데이트 전송
			SendPartyUpdateFromRoom(gameRoom, partyId);
		});
}

void ServerPacketHandler::Handle_C_PartyLeave(GameSessionRef session, BYTE* buffer, int32 len)
{
	GameRoomRef gameRoom = session->gameRoom.lock();
	if (!gameRoom)
		return;

	gameRoom->PushJob([session, gameRoom]()
		{
			PlayerRef player = dynamic_pointer_cast<Player>(session->player.lock());
			if (!player)
				return;

			uint64 playerId = player->info.objectid();
			uint64 partyId = GPartyManager.GetPartyIdByPlayer(playerId);
			if (partyId == 0)
				return;

			// 탈퇴 전에 파티 멤버 목록 복사 (DisbandParty가 호출될 수 있으므로)
			Party* party = GPartyManager.GetParty(partyId);
			if (!party)
				return;
			vector<uint64> remainingMembers = party->memberIds;

			GPartyManager.RemoveMember(partyId, playerId);

			// 탈퇴자에게 S_PartyLeave
			{
				Protocol::S_PartyLeave leavePkt;
				SendBufferRef sendBuffer = MakeSendBuffer(leavePkt, S_PartyLeave);
				session->Send(sendBuffer);
			}

			// 파티가 아직 존재하면 남은 멤버에게 업데이트
			Party* remainingParty = GPartyManager.GetParty(partyId);
			if (remainingParty)
			{
				SendPartyUpdateFromRoom(gameRoom, partyId);
			}
			else
			{
				// 파티 해산됨 → 남은 멤버에게 S_PartyLeave
				for (uint64 memberId : remainingMembers)
				{
					if (memberId == playerId)
						continue;
					GameObjectRef obj = gameRoom->FindObject(memberId);
					if (!obj)
						continue;
					PlayerRef p = dynamic_pointer_cast<Player>(obj);
					if (p && p->session)
					{
						Protocol::S_PartyLeave leavePkt;
						SendBufferRef sendBuffer = MakeSendBuffer(leavePkt, S_PartyLeave);
						p->session->Send(sendBuffer);
					}
				}
			}
		});
}

SendBufferRef ServerPacketHandler::Make_S_PartyInvite(uint64 inviterId, const string& inviterName)
{
	Protocol::S_PartyInvite pkt;
	pkt.set_inviterid(inviterId);
	pkt.set_invitername(inviterName);
	return MakeSendBuffer(pkt, S_PartyInvite);
}

SendBufferRef ServerPacketHandler::Make_S_PartyUpdate(const vector<Protocol::PartyMemberInfo>& members)
{
	Protocol::S_PartyUpdate pkt;
	for (const auto& m : members)
		*pkt.add_members() = m;
	return MakeSendBuffer(pkt, S_PartyUpdate);
}

SendBufferRef ServerPacketHandler::Make_S_PartyLeave()
{
	Protocol::S_PartyLeave pkt;
	return MakeSendBuffer(pkt, S_PartyLeave);
}

// ---- Login ----

void ServerPacketHandler::Handle_C_Login(GameSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	uint16 size = header->size;

	Protocol::C_Login pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	string username = pkt.username();
	if (username.empty())
		return;

	// DB에서 계정 조회/생성
	int64 accountId = GDBManager.FindOrCreateAccount(username);
	if (accountId == 0)
		return;

	session->_accountId = accountId;

	// 플레이어 생성
	PlayerRef player = GameObject::CreatePlayer();

	if (GDBManager.HasPlayerData(accountId))
	{
		// 기존 데이터 로드
		PlayerSaveData saveData;
		GDBManager.LoadPlayerData(accountId, saveData);
		player->ApplyFromSaveData(saveData);
	}
	else
	{
		// 새 계정: username을 이름으로, 초기 데이터 저장
		player->info.set_name(username);
		PlayerSaveData initData = player->ToSaveData();
		GDBManager.SavePlayerData(accountId, initData);
	}

	// S_EnterGame 전송
	{
		Protocol::S_EnterGame enterPkt;
		enterPkt.set_success(true);
		enterPkt.set_accountid(accountId);
		session->Send(MakeSendBuffer(enterPkt, S_EnterGame));
	}

	// 마을에 입장
	GameRoomRef room = GRoomManager.GetStaticRoom(FieldId::Town, 1);
	if (room)
	{
		room->PushJob([room, session, player]()
			{
				room->EnterRoom(session, player);
			});
	}

	cout << "[Login] " << username << " logged in (accountId=" << accountId << ")" << endl;
}
