#include "pch.h"
#include "MyPlayer.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "CameraComponent.h"
#include "SceneManager.h"
#include "DevScene.h"
#include "HitEffect.h"
#include "Arrow.h"

MyPlayer::MyPlayer ( )
{
	CameraComponent* camera = new CameraComponent ( );
	AddComponent ( camera );
}

MyPlayer::~MyPlayer ( )
{
}

void MyPlayer::BeginPlay ( )
{
	Super::BeginPlay ( );
}

void MyPlayer::Tick ( )
{
	Super::Tick ( );

	float dt = GET_SINGLE ( TimeManager )->GetDeltaTime ( );
	if ( dt > 0.05f ) dt = 0.05f;

	if ( _turnGraceLeft > 0.f )
		_turnGraceLeft -= dt;

	float now = GET_SINGLE ( TimeManager )->GetTime ( );
	if ( _attackPending && ( now - _attackPendingStart ) > 0.25f )
		_attackPending = false;
}

void MyPlayer::Render ( HDC hdc )
{
	Super::Render (hdc );
}

void MyPlayer::TickInput ( )
{
	if ( GET_SINGLE ( InputManager )->IsInputLocked ( ) )
		return;

	_keyPressed = true;

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::W ) )
		_wantedDir = DIR_UP;
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::S ) )
		_wantedDir = DIR_DOWN;
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::A ) )
		_wantedDir = DIR_LEFT;
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::D ) )
		_wantedDir = DIR_RIGHT;
	else
		_keyPressed = false;

	if ( _keyPressed == false )
		_blockedHold = false;

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::KEY_1 ) )
		SetWeaponType ( WeaponType::Sword );
	else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::KEY_2 ) )
		SetWeaponType ( WeaponType::Bow );
	else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::KEY_3 ) )
		SetWeaponType ( WeaponType::Staff );

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::SpaceBar ) )
	{
		float now = GET_SINGLE ( TimeManager )->GetTime ( );

		if ( now < _attackCooldownUntil ) return;
		if ( _attackPending ) return;

		// 마을에서는 공격 불가
		Scene* cur = GET_SINGLE ( SceneManager )->GetCurrentScene ( );
		DevScene* scene = dynamic_cast< DevScene* >( cur );
		if ( scene == nullptr )
			return;

		if ( scene->HasMapId ( ) == false )
			return;

		if ( scene->IsTown ( ) )
			return;
		
		TrySkill ( );
		_attackPending = true;
		_attackPendingStart = now;
		_attackCooldownUntil = now + 0.15f;
	}

}


void MyPlayer::TryMove ( )
{
	if ( _keyPressed == false )
		return;

	if ( _movePending )
		return;

	const auto currentDir = info.dir ( );
	const auto wantedDir = _wantedDir;

	if ( _blockedHold && currentDir == wantedDir && _blockedDir == wantedDir )
		return;

	// 방향이 다르면 Turn
	if ( currentDir != wantedDir )
	{
		Protocol::C_Turn turnPkt;
		turnPkt.set_dir ( wantedDir );
		SendBufferRef sb = ClientPacketHandler::Make_C_Turn ( turnPkt );
		GET_SINGLE ( NetworkManager )->SendPacket ( sb );

		_keyPressed = false;
		_movePending = true;
		return;
	}

	if ( _turnGraceLeft > 0.f )
		return;

	_lastMoveFrom = GetCellPos ( );
	_lastMoveDir = currentDir;
	_hasLastMoveRequest = true;

	// 방향이 같으면 Move
	Protocol::C_Move movePkt;
	movePkt.set_dir ( currentDir );
	SendBufferRef sb = ClientPacketHandler::Make_C_Move ( movePkt );
	GET_SINGLE ( NetworkManager )->SendPacket ( sb );

	_keyPressed = false;
	_movePending = true;
}


void MyPlayer::TrySkill ( )
{
	Protocol::WEAPON_TYPE weapon = ToProtoWeaponType(GetWeaponType ( ));
	Protocol::C_Attack attackPkt;
	attackPkt.set_dir ( info.dir ( ) );
	attackPkt.set_weapontype ( weapon );
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Attack ( attackPkt );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void MyPlayer::TickIdle ( )
{
	TickInput ( );
	TryMove ( );
}

void MyPlayer::TickMove ( )
{
	Super::TickMove ( );
}

void MyPlayer::TickSkill ( )
{
	Super::TickSkill ( );
}

InventorySlot* MyPlayer::GetEquipSlot ( int32 equipType )
{
	if ( equipType == 0 ) return &_equipWeapon;
	if ( equipType == 1 ) return &_equipArmor;
	if ( equipType == 2 ) return &_equipPotion;
	return nullptr;
}

void MyPlayer::ApplyInventoryData ( const Protocol::S_InventoryData& pkt )
{
	for ( int32 i = 0; i < INVENTORY_SIZE; i++ )
	{
		_storage[ i ].itemId = 0;
		_storage[ i ].count = 0;
	}

	for ( int32 i = 0; i < pkt.items_size ( ); i++ )
	{
		const auto& item = pkt.items ( i );
		int32 slot = item.slot ( );
		if ( slot >= 0 && slot < INVENTORY_SIZE )
		{
			_storage[ slot ].itemId = item.itemid ( );
			_storage[ slot ].count = item.count ( );
		}
	}

	_equipWeapon.itemId = pkt.equippedweapon ( ).itemid ( );
	_equipWeapon.count = pkt.equippedweapon ( ).count ( );
	_equipArmor.itemId = pkt.equippedarmor ( ).itemid ( );
	_equipArmor.count = pkt.equippedarmor ( ).count ( );
	_equipPotion.itemId = pkt.equippedpotion ( ).itemid ( );
	_equipPotion.count = pkt.equippedpotion ( ).count ( );
}

void MyPlayer::ApplyAddItem ( int32 slot, int32 itemId, int32 count )
{
	if ( slot >= 0 && slot < INVENTORY_SIZE )
	{
		_storage[ slot ].itemId = itemId;
		_storage[ slot ].count = count;
	}
}

void MyPlayer::ApplyEquipItem ( const Protocol::S_EquipItem& pkt )
{
	int32 slot = pkt.storageslot ( );
	if ( slot >= 0 && slot < INVENTORY_SIZE )
	{
		_storage[ slot ].itemId = pkt.storageitemid ( );
		_storage[ slot ].count = pkt.storageitemcount ( );
	}

	if ( InventorySlot* equipSlot = GetEquipSlot ( pkt.equiptype ( ) ) )
	{
		equipSlot->itemId = pkt.equipitemid ( );
		equipSlot->count = pkt.equipitemcount ( );
	}

	info.set_attack ( pkt.attack ( ) );
	info.set_defence ( pkt.defence ( ) );
}

void MyPlayer::ApplyUnequipItem ( const Protocol::S_UnequipItem& pkt )
{
	if ( InventorySlot* equipSlot = GetEquipSlot ( pkt.equiptype ( ) ) )
	{
		int32 slot = pkt.storageslot ( );
		if ( slot >= 0 && slot < INVENTORY_SIZE )
		{
			_storage[ slot ].itemId = equipSlot->itemId;
			_storage[ slot ].count = equipSlot->count;
		}
		equipSlot->itemId = 0;
		equipSlot->count = 0;
	}

	info.set_attack ( pkt.attack ( ) );
	info.set_defence ( pkt.defence ( ) );
}

void MyPlayer::ApplyUseItem ( const Protocol::S_UseItem& pkt )
{
	if ( InventorySlot* slot = GetEquipSlot ( pkt.equiptype ( ) ) )
	{
		slot->count = pkt.remaincount ( );
		if ( slot->count <= 0 )
		{
			slot->itemId = 0;
			slot->count = 0;
		}
	}

	info.set_hp ( pkt.newhp ( ) );
}

void MyPlayer::OnServerMoveResult ( const Protocol::ObjectInfo& serverInfo )
{
	_movePending = false;

	if ( _hasLastMoveRequest == false )
		return;

	uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );
	if ( myId != serverInfo.objectid ( ) )
		return;

	Vec2Int newPos{ serverInfo.posx ( ), serverInfo.posy ( ) };
	bool stayed = ( newPos == _lastMoveFrom );
	if ( stayed && serverInfo.state ( ) == IDLE )
	{
		_blockedHold = true;
		_blockedDir = _lastMoveDir;
	}
	else
	{
		_blockedHold = false;
	}

	_hasLastMoveRequest = false;
}

