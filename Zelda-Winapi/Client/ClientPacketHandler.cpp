#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "DevScene.h"
#include "MyPlayer.h"
#include "SceneManager.h"
#include "HitEffect.h"

void ClientPacketHandler::HandlePacket( ServerSessionRef session , BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	switch (header.id)
	{
	case S_TEST:
		Handle_S_TEST( session, buffer, len);
		break;
	case S_EnterGame:
		Handle_S_EnterGame ( session, buffer , len );
		break;
	case S_MyPlayer:
		Handle_S_MyPlayer ( session , buffer , len );
		break;
	case S_AddObject:
		Handle_S_AddObject ( session , buffer , len );
		break;
	case S_RemoveObject:
		Handle_S_RemoveObject ( session , buffer , len );
		break;
	case S_Move:
		Handle_S_Move ( session , buffer , len );
		break;
	case S_Attack:
		Handle_S_Attack ( session , buffer , len );
		break;
	case S_Damaged:
		Handle_S_Damaged ( session , buffer , len );
		break;
	case S_ChangeMap:
		Handle_S_ChangeMap ( session , buffer , len );
		break;
	// [AUTO-GEN SWITCH BEGIN]


	// [AUTO-GEN SWITCH END]
	default:
		break;
	}
}

void ClientPacketHandler::Handle_S_TEST( ServerSessionRef session, BYTE* buffer, int32 len)
{
	PacketHeader* header = (PacketHeader*)buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_TEST pkt;
	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

	uint64 id = pkt.id();
	uint32 hp = pkt.hp();
	uint16 attack = pkt.attack();

	for (int32 i = 0; i < pkt.buffs_size(); i++)
	{
		const Protocol::BuffData& data = pkt.buffs(i);
	}
}

void ClientPacketHandler::Handle_S_EnterGame ( ServerSessionRef session, BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_EnterGame pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	bool success = pkt.success ( );
	uint64 accountId = pkt.accountid ( );

	// TODO
}

void ClientPacketHandler::Handle_S_MyPlayer ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_MyPlayer pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	///
	const Protocol::ObjectInfo& info = pkt.info ( );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
	{
		MyPlayer* myPlayer = scene->SpawnObject<MyPlayer> ( Vec2Int{ info.posx ( ), info.posy ( ) } );
		myPlayer->info = info;
		GET_SINGLE ( SceneManager )->SetMyPlayer ( myPlayer );
	}
}

void ClientPacketHandler::Handle_S_AddObject ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_AddObject pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
		scene->Handle_S_AddObject ( pkt);
}

void ClientPacketHandler::Handle_S_RemoveObject ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_RemoveObject pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
		scene->Handle_S_RemoveObject ( pkt );
}

void ClientPacketHandler::Handle_S_Move ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_Move pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	//

	const Protocol::ObjectInfo& info = pkt.info ( );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
	{
		uint64 myPlayerId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );
		if ( myPlayerId == info.objectid ( ) )
			return;

		GameObject* gameObject = scene->GetObjectW ( info.objectid ( ) );
		if (gameObject )
		{
			gameObject->SetDir ( info.dir ( ) );
			gameObject->SetState ( info.state ( ) );
			gameObject->SetCellPos ( Vec2Int{ info.posx ( ) , info.posy ( ) } );
		}
	}
}

void ClientPacketHandler::Handle_S_Attack ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_Attack pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	//
	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
	{
		GameObject* gameObject = scene->GetObjectW ( pkt.attackerid ( ) );
		if ( gameObject )
		{
			gameObject->SetDir ( pkt.dir ( ) );

			if ( Player* player = dynamic_cast< Player* >( gameObject ) )
				player->SetWeaponType ( Player::FromProtoWeaponType ( pkt.weapontype ( ) ) );

			gameObject->SetState ( SKILL );
		}
	}
}

void ClientPacketHandler::Handle_S_Damaged ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_Damaged pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	//
	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
	{
		// GameObject* attackerGameObject = scene->GetObjectW ( pkt.attackerid ( ) );
		GameObject* targetGameObject = scene->GetObjectW ( pkt.targetid ( ) );
		// int32 damage = pkt.damage ( );
		if ( targetGameObject )
		{
			if ( Creature* creature = dynamic_cast< Creature* >( targetGameObject ) )
			{
				creature->info.set_hp ( pkt.newhp ( ) );
				scene->SpawnObject<HitEffect> ( targetGameObject->GetCellPos ( ) );
			}
		}
	}
}

void ClientPacketHandler::Handle_S_ChangeMap ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_ChangeMap pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	if ( pkt.success ( ) == false )
	{
		// 실패
		return;
	}

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene == nullptr )
		return;

	scene->ChangeMap ( pkt.mapid ( ) );
	GET_SINGLE ( SceneManager )->SetMyPlayer ( nullptr );
}


SendBufferRef ClientPacketHandler::Make_C_Move ( Protocol::DIR_TYPE dir , int32 x , int32 y  )
{
	Protocol::C_Move pkt;
	pkt.set_dir ( dir );
	pkt.set_targetx ( x );
	pkt.set_targety ( y );

	return MakeSendBuffer ( pkt , C_Move );
}

SendBufferRef ClientPacketHandler::Make_C_Attack ( Protocol::DIR_TYPE dir , Protocol::WEAPON_TYPE weapon )
{
	Protocol::C_Attack pkt;
	pkt.set_dir ( dir );
	pkt.set_weapontype ( weapon );

	return MakeSendBuffer ( pkt , C_Attack );
}

SendBufferRef ClientPacketHandler::Make_C_ChangeMap ( const Protocol::MAP_ID& mapId , int32 channel )
{
	Protocol::C_ChangeMap pkt;
	pkt.set_mapid ( mapId );
	pkt.set_channel ( channel );

	return MakeSendBuffer ( pkt , C_ChangeMap );
}
