#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "DevScene.h"
#include "MyPlayer.h"
#include "SceneManager.h"
#include "HitEffect.h"
#include "SoundManager.h"

void ClientPacketHandler::HandlePacket( ServerSessionRef session , BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	switch (header.id)
	{
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
	case S_Chat:
		Handle_S_Chat ( session , buffer , len );
		break;
	case S_Ranking:
		Handle_S_Ranking ( session , buffer , len );
		break;
	// [AUTO-GEN SWITCH BEGIN]

	// [AUTO-GEN SWITCH END]
	default:
		break;
	}
}

static wstring ToWstring ( const string& utf8 )
{
	int32 size = ::MultiByteToWideChar ( CP_UTF8 , 0 , utf8.c_str ( ) , -1 , nullptr , 0 );
	wstring result ( size - 1 , 0 );
	::MultiByteToWideChar ( CP_UTF8 , 0 , utf8.c_str ( ) , -1 , &result[ 0 ] , size );
	return result;
}

// **** HANDLE ****

void ClientPacketHandler::Handle_S_EnterGame ( ServerSessionRef session, BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_EnterGame pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	bool success = pkt.success ( );
	if ( success )
	{
		DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
		if ( scene )
			scene->SetLoggedIn ( true );
	}
}

void ClientPacketHandler::Handle_S_MyPlayer ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_MyPlayer pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

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
	uint16 size = header->size;

	Protocol::S_Move pkt;
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
			// pending/blocked 상태 갱신
			mp->OnServerMoveResult ( info );
		}
	}

	gameObject->SetDir ( info.dir ( ) );
	gameObject->SetState ( info.state ( ) );
	gameObject->SetCellPos ( Vec2Int{ info.posx ( ) , info.posy ( ) } );
}

void ClientPacketHandler::Handle_S_Attack ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_Attack pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
		scene->Handle_S_Attack ( pkt );
}

void ClientPacketHandler::Handle_S_Damaged ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_Damaged pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene )
	{
		GameObject* targetGameObject = scene->GetObjectW ( pkt.targetid ( ) );
		if ( targetGameObject )
		{
			if ( Creature* creature = dynamic_cast< Creature* >( targetGameObject ) )
			{
				creature->info.set_hp ( pkt.newhp ( ) );
				scene->SpawnObject<HitEffect> ( targetGameObject->GetCellPos ( ) );
			}
		}

		// 파티 HUD HP 실시간 갱신
		MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
		if ( myPlayer )
		{
			for ( auto& member : myPlayer->_partyMembers )
			{
				if ( member.playerId == pkt.targetid ( ) )
				{
					member.hp = pkt.newhp ( );
					break;
				}
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
	uint16 size = header->size;

	Protocol::S_GainExp pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

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
	uint16 size = header->size;

	Protocol::S_LevelUp pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

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

void ClientPacketHandler::Handle_S_InventoryData ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	uint16 size = header->size;

	Protocol::S_InventoryData pkt;
	pkt.ParseFromArray ( &header[ 1 ] , size - sizeof ( PacketHeader ) );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	myPlayer->ApplyInventoryData ( pkt );
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

	myPlayer->ApplyAddItem ( pkt.slot ( ) , pkt.itemid ( ) , pkt.count ( ) );
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

	myPlayer->ApplyEquipItem ( pkt );
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

	myPlayer->ApplyUnequipItem ( pkt );
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

	myPlayer->ApplyUseItem ( pkt );
}

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
	GET_SINGLE ( SoundManager )->Play ( L"UISound" );

	myPlayer->_pendingInviterName = ToWstring ( pkt.invitername ( ) );
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
		data.name = ToWstring ( m.name ( ) );
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
				data.name = ToWstring ( obj->info.name ( ) );
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
	if ( myPlayer->_pendingInviteFrom != 0 )
	{
		myPlayer->_pendingInviteFrom = 0;
		myPlayer->_pendingInviterName.clear ( );
		GET_SINGLE ( SoundManager )->Play ( L"UISound" );
	}
}

void ClientPacketHandler::Handle_S_Chat ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	Protocol::S_Chat pkt;
	pkt.ParseFromArray ( &header[ 1 ] , len - sizeof ( PacketHeader ) );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene == nullptr )
		return;

	wstring sender = ToWstring ( pkt.sender ( ) );
	wstring msg = ToWstring ( pkt.msg ( ) );

	wstring prefix;
	switch ( pkt.type ( ) )
	{
		case Protocol::CHAT_TYPE_GLOBAL:  prefix = L"[전체] "; break;
		case Protocol::CHAT_TYPE_PARTY:   prefix = L"[파티] "; break;
		case Protocol::CHAT_TYPE_WHISPER: prefix = L"[귓말] "; break;
		default: break;
	}

	scene->AddChatMessage ( prefix + sender , msg );
}

void ClientPacketHandler::Handle_S_Ranking ( ServerSessionRef session , BYTE* buffer , int32 len )
{
	PacketHeader* header = ( PacketHeader* ) buffer;
	Protocol::S_Ranking pkt;
	pkt.ParseFromArray ( &header[ 1 ] , len - sizeof ( PacketHeader ) );

	DevScene* scene = GET_SINGLE ( SceneManager )->GetDevScene ( );
	if ( scene == nullptr )
		return;

	vector<pair<wstring, int32>> entries;
	for ( const auto& e : pkt.entries ( ) )
		entries.push_back ( { ToWstring ( e.playername ( ) ), e.level ( ) } );

	scene->SetRankingData ( entries );
}


// **** MAKE ****

SendBufferRef ClientPacketHandler::Make_C_Move ( const Protocol::C_Move& pkt )
{
	return MakeSendBuffer ( pkt , C_Move );
}

SendBufferRef ClientPacketHandler::Make_C_Attack ( const Protocol::C_Attack& pkt )
{
	return MakeSendBuffer ( pkt , C_Attack );
}

SendBufferRef ClientPacketHandler::Make_C_ChangeMap ( const Protocol::C_ChangeMap& pkt )
{
	return MakeSendBuffer ( pkt , C_ChangeMap );
}

SendBufferRef ClientPacketHandler::Make_C_Turn ( const Protocol::C_Turn& pkt )
{
	return MakeSendBuffer ( pkt , C_Turn );
}

SendBufferRef ClientPacketHandler::Make_C_EquipItem ( const Protocol::C_EquipItem& pkt )
{
	return MakeSendBuffer ( pkt , C_EquipItem );
}

SendBufferRef ClientPacketHandler::Make_C_UnequipItem ( const Protocol::C_UnequipItem& pkt )
{
	return MakeSendBuffer ( pkt , C_UnequipItem );
}

SendBufferRef ClientPacketHandler::Make_C_UseItem ( const Protocol::C_UseItem& pkt )
{
	return MakeSendBuffer ( pkt , C_UseItem );
}

SendBufferRef ClientPacketHandler::Make_C_PartyInvite ( const Protocol::C_PartyInvite& pkt )
{
	return MakeSendBuffer ( pkt , C_PartyInvite );
}

SendBufferRef ClientPacketHandler::Make_C_PartyAnswer ( const Protocol::C_PartyAnswer& pkt )
{
	return MakeSendBuffer ( pkt , C_PartyAnswer );
}

SendBufferRef ClientPacketHandler::Make_C_PartyLeave ( )
{
	Protocol::C_PartyLeave pkt;
	return MakeSendBuffer ( pkt , C_PartyLeave );
}

SendBufferRef ClientPacketHandler::Make_C_Login ( const Protocol::C_Login& pkt )
{
	return MakeSendBuffer ( pkt , C_Login );
}

SendBufferRef ClientPacketHandler::Make_C_Chat ( const Protocol::C_Chat& pkt )
{
	return MakeSendBuffer ( pkt , C_Chat );
}

SendBufferRef ClientPacketHandler::Make_C_GetRanking ( )
{
	Protocol::C_GetRanking pkt;
	return MakeSendBuffer ( pkt , C_GetRanking );
}
