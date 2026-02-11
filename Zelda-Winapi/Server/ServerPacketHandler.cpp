#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameRoom.h"
#include "GameRoomManager.h"


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
	// [AUTO-GEN SWITCH BEGIN]




		





		

	// [AUTO-GEN SWITCH END]
	default:
		break;
	}
}

int a = 0;
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

	cout << "Handle_C_Move Count " << ++a << endl;
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

	fromRoom->PushJob([session, pkt]()
		{
			GameRoomRef from = session->gameRoom.lock();
			if (!from)
				return;

			GameRoomRef to = nullptr;
			uint64 instanceId = 0;

			if (pkt.mapid() == Protocol::MAP_ID_TOWN)
			{
				to = GRoomManager.GetStaticRoom(FieldId::Town, pkt.channel());
			}
			else if (pkt.mapid() == Protocol::MAP_ID_DUNGEON)
			{
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

			{
				Protocol::S_ChangeMap sendPkt;
				sendPkt.set_success(true);
				sendPkt.set_mapid(pkt.mapid());
				sendPkt.set_channel(pkt.channel());
				sendPkt.set_instanceid(instanceId);

				session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
			}

			from->LeaveRoom(session);

			to->PushJob([to, session]()
				{
					to->EnterRoom(session);
				});
		});
}

int b = 0;
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

	cout << "Handle_C_Turn Count " << ++b << endl;
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
