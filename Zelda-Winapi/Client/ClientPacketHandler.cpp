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
	case S_GainExp:
		Handle_S_GainExp ( session , buffer , len );
		break;
	case S_LevelUp:
		Handle_S_LevelUp ( session , buffer , len );
		break;
	case S_Turn:
		Handle_S_Turn ( session , buffer , len );
		break;
	case S_InventoryData:
		Handle_S_InventoryData ( session , buffer , len );
		break;
	case S_AddItem:
		Handle_S_AddItem ( session , buffer , len );
		break;
	case S_EquipItem:
		Handle_S_EquipItem ( session , buffer , len );
		break;
	case S_UnequipItem:
		Handle_S_UnequipItem ( session , buffer , len );
		break;
	case S_UseItem:
		Handle_S_UseItem ( session , buffer , len );
		break;
	case S_PartyInvite:
		Handle_S_PartyInvite ( session , buffer , len );
		break;
	case S_PartyUpdate:
		Handle_S_PartyUpdate ( session , buffer , len );
		break;
	case S_PartyLeave:
		Handle_S_PartyLeave ( session , buffer , len );
		break;
	// [AUTO-GEN SWITCH BEGIN]


		


	// [AUTO-GEN SWITCH END]
	default:
		break;
	}
}

// **** HANDLE ****

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
	if ( scene == nullptr )
		return;

	GameObject* gameObject = scene->GetObjectW ( info.objectid ( ) );
	if ( gameObject == nullptr )
		return;

	uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );
	if ( myId == info.objectid ( ) )
	{
		if ( info.state ( ) == IDLE )
		{
			if ( auto mp = dynamic_cast< MyPlayer* >( gameObject ) )
			{
				mp->OnServerMoveEndAck ( );
			}

		}
	}

	gameObject->SetDir ( info.dir ( ) );
	gameObject->SetState ( info.state ( ) );
	gameObject->SetCellPos ( Vec2Int{ info.posx ( ) , info.posy ( ) } );
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
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_ChangeMap pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	//

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

void ClientPacketHandler::Handle_S_GainExp ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_GainExp pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	//

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	if ( myPlayer->info.objectid ( ) != pkt.playerid ( ) )
		return;

	// PlayerExtra 갱신
	Protocol::PlayerExtra* extra = myPlayer->info.mutable_player ( );
	extra->set_exp ( pkt.currentexp ( ) );
	extra->set_maxexp ( pkt.maxexp ( ) );
}

void ClientPacketHandler::Handle_S_LevelUp ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	//uint16 id = header->id;
	uint16 size = header->size;

	Protocol::S_LevelUp pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	//

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene == nullptr )
		return;

	GameObject* gameObject = scene->GetObjectW ( pkt.playerid ( ) );
	if ( gameObject == nullptr )
		return;

	Creature* creature = dynamic_cast< Creature* >( gameObject );
	if ( creature == nullptr )
		return;

	// 스탯 업데이트
	creature->info.set_maxhp ( pkt.maxhp ( ) );
	creature->info.set_hp ( pkt.maxhp ( ) );  // 레벨업 시 HP 전체 회복
	creature->info.set_attack ( pkt.attack ( ) );
	creature->info.set_defence ( pkt.defence ( ) );

	// PlayerExtra 갱신
	Protocol::PlayerExtra* extra = creature->info.mutable_player ( );
	extra->set_level ( pkt.newlevel ( ) );
	extra->set_maxexp ( pkt.maxexp ( ) );
}

void ClientPacketHandler::Handle_S_Turn ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_Turn pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	const Protocol::ObjectInfo& info = pkt.info ( );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene == nullptr )
		return;

	GameObject* gameObject = scene->GetObjectW ( info.objectid ( ) );
	if ( gameObject == nullptr )
		return;

	uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );
	if ( myId == info.objectid ( ) )
	{
		if ( auto mp = dynamic_cast< MyPlayer* >( gameObject ) )
		{
			mp->OnServerTurnAck ( );
		}
	}

	gameObject->SetDir ( info.dir ( ) );
	gameObject->SetState ( info.state ( ) );
}



// **** MAKE ****

SendBufferRef ClientPacketHandler::Make_C_Move ( Protocol::DIR_TYPE dir )
{
	Protocol::C_Move pkt;
	pkt.set_dir ( dir );

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

SendBufferRef ClientPacketHandler::Make_C_Turn ( const Protocol::DIR_TYPE& dir )
{
	Protocol::C_Turn pkt;
	pkt.set_dir ( dir );

	return MakeSendBuffer ( pkt , C_Turn );
}

void ClientPacketHandler::Handle_S_InventoryData ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_InventoryData pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	// 초기화
	for ( int32 i = 0; i < MyPlayer::INVENTORY_SIZE; i++ )
	{
		myPlayer->_storage[ i ].itemId = 0;
		myPlayer->_storage[ i ].count = 0;
	}

	for ( int32 i = 0; i < pkt.items_size ( ); i++ )
	{
		const auto& item = pkt.items ( i );
		int32 slot = item.slot ( );
		if ( slot >= 0 && slot < MyPlayer::INVENTORY_SIZE )
		{
			myPlayer->_storage[ slot ].itemId = item.itemid ( );
			myPlayer->_storage[ slot ].count = item.count ( );
		}
	}

	myPlayer->_equipWeapon.itemId = pkt.equippedweapon ( ).itemid ( );
	myPlayer->_equipWeapon.count = pkt.equippedweapon ( ).count ( );
	myPlayer->_equipArmor.itemId = pkt.equippedarmor ( ).itemid ( );
	myPlayer->_equipArmor.count = pkt.equippedarmor ( ).count ( );
	myPlayer->_equipPotion.itemId = pkt.equippedpotion ( ).itemid ( );
	myPlayer->_equipPotion.count = pkt.equippedpotion ( ).count ( );
}

void ClientPacketHandler::Handle_S_AddItem ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_AddItem pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	int32 slot = pkt.slot ( );
	if ( slot >= 0 && slot < MyPlayer::INVENTORY_SIZE )
	{
		myPlayer->_storage[ slot ].itemId = pkt.itemid ( );
		myPlayer->_storage[ slot ].count = pkt.count ( );
	}
}

void ClientPacketHandler::Handle_S_EquipItem ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_EquipItem pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	int32 slot = pkt.storageslot ( );
	if ( slot >= 0 && slot < MyPlayer::INVENTORY_SIZE )
	{
		myPlayer->_storage[ slot ].itemId = pkt.storageitemid ( );
		myPlayer->_storage[ slot ].count = pkt.storageitemcount ( );
	}

	int32 equipType = pkt.equiptype ( );
	InventorySlot* equipSlot = nullptr;
	if ( equipType == 0 )
		equipSlot = &myPlayer->_equipWeapon;
	else if ( equipType == 1 )
		equipSlot = &myPlayer->_equipArmor;
	else if ( equipType == 2 )
		equipSlot = &myPlayer->_equipPotion;

	if ( equipSlot )
	{
		equipSlot->itemId = pkt.equipitemid ( );
		equipSlot->count = pkt.equipitemcount ( );
	}

	myPlayer->info.set_attack ( pkt.attack ( ) );
	myPlayer->info.set_defence ( pkt.defence ( ) );
}

void ClientPacketHandler::Handle_S_UnequipItem ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_UnequipItem pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	int32 equipType = pkt.equiptype ( );
	InventorySlot* equipSlot = nullptr;
	if ( equipType == 0 )
		equipSlot = &myPlayer->_equipWeapon;
	else if ( equipType == 1 )
		equipSlot = &myPlayer->_equipArmor;
	else if ( equipType == 2 )
		equipSlot = &myPlayer->_equipPotion;

	if ( equipSlot )
	{
		// 기존 장비를 storage로 이동
		int32 slot = pkt.storageslot ( );
		if ( slot >= 0 && slot < MyPlayer::INVENTORY_SIZE )
		{
			myPlayer->_storage[ slot ].itemId = equipSlot->itemId;
			myPlayer->_storage[ slot ].count = equipSlot->count;
		}
		equipSlot->itemId = 0;
		equipSlot->count = 0;
	}

	myPlayer->info.set_attack ( pkt.attack ( ) );
	myPlayer->info.set_defence ( pkt.defence ( ) );
}

void ClientPacketHandler::Handle_S_UseItem ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_UseItem pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	int32 equipType = pkt.equiptype ( );
	if ( equipType == 2 )
	{
		myPlayer->_equipPotion.count = pkt.remaincount ( );
		if ( myPlayer->_equipPotion.count <= 0 )
		{
			myPlayer->_equipPotion.itemId = 0;
			myPlayer->_equipPotion.count = 0;
		}
	}

	myPlayer->info.set_hp ( pkt.newhp ( ) );
}

SendBufferRef ClientPacketHandler::Make_C_EquipItem ( int32 slot )
{
	Protocol::C_EquipItem pkt;
	pkt.set_slot ( slot );
	return MakeSendBuffer ( pkt , C_EquipItem );
}

SendBufferRef ClientPacketHandler::Make_C_UnequipItem ( int32 equipType )
{
	Protocol::C_UnequipItem pkt;
	pkt.set_equiptype ( equipType );
	return MakeSendBuffer ( pkt , C_UnequipItem );
}

SendBufferRef ClientPacketHandler::Make_C_UseItem ( int32 slot )
{
	Protocol::C_UseItem pkt;
	pkt.set_slot ( slot );
	return MakeSendBuffer ( pkt , C_UseItem );
}

// ---- Party ----

void ClientPacketHandler::Handle_S_PartyInvite ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_PartyInvite pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	myPlayer->_pendingInviteFrom = pkt.inviterid ( );

	// string → wstring 변환
	string name = pkt.invitername ( );
	myPlayer->_pendingInviterName = wstring ( name.begin ( ) , name.end ( ) );
}

void ClientPacketHandler::Handle_S_PartyUpdate ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_PartyUpdate pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	myPlayer->_partyMembers.clear ( );

	for ( int32 i = 0; i < pkt.members_size ( ); i++ )
	{
		const auto& m = pkt.members ( i );
		PartyMemberData data;
		data.playerId = m.playerid ( );
		string name = m.name ( );
		data.name = wstring ( name.begin ( ) , name.end ( ) );
		data.level = m.level ( );
		data.hp = m.hp ( );
		data.maxHp = m.maxhp ( );
		data.isLeader = m.isleader ( );

		// 같은 방에 있는 플레이어에서 이름/레벨/HP 보완
		DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
		if ( scene )
		{
			GameObject* obj = scene->GetObjectW ( data.playerId );
			if ( obj )
			{
				string objName = obj->info.name ( );
				data.name = wstring ( objName.begin ( ) , objName.end ( ) );
				data.hp = obj->info.hp ( );
				data.maxHp = obj->info.maxhp ( );
				data.level = obj->info.player ( ).level ( );
			}
		}

		myPlayer->_partyMembers.push_back ( data );
	}
}

void ClientPacketHandler::Handle_S_PartyLeave ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	myPlayer->_partyMembers.clear ( );
}

SendBufferRef ClientPacketHandler::Make_C_PartyInvite ( uint64 targetId )
{
	Protocol::C_PartyInvite pkt;
	pkt.set_targetid ( targetId );
	return MakeSendBuffer ( pkt , C_PartyInvite );
}

SendBufferRef ClientPacketHandler::Make_C_PartyAnswer ( uint64 inviterId , bool accept )
{
	Protocol::C_PartyAnswer pkt;
	pkt.set_inviterid ( inviterId );
	pkt.set_accept ( accept );
	return MakeSendBuffer ( pkt , C_PartyAnswer );
}

SendBufferRef ClientPacketHandler::Make_C_PartyLeave ( )
{
	Protocol::C_PartyLeave pkt;
	return MakeSendBuffer ( pkt , C_PartyLeave );
}
