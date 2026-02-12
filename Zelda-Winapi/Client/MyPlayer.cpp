#include "pch.h"
#include "MyPlayer.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "CameraComponent.h"
#include "BoxCollider.h"
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

	// 방향이 다르면 Turn
	if ( currentDir != wantedDir )
	{
		SendBufferRef sb = ClientPacketHandler::Make_C_Turn ( wantedDir );
		GET_SINGLE ( NetworkManager )->SendPacket ( sb );

		_keyPressed = false;
		_movePending = true;
		return;
	}

	if ( _turnGraceLeft > 0.f )
		return;

	// 방향이 같으면 Move
	SendBufferRef sb = ClientPacketHandler::Make_C_Move ( currentDir );
	GET_SINGLE ( NetworkManager )->SendPacket ( sb );

	_keyPressed = false;
	_movePending = true;
}


void MyPlayer::TrySkill ( )
{
	Protocol::WEAPON_TYPE weapon = ToProtoWeaponType(GetWeaponType ( ));
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Attack ( info.dir ( ) , weapon );
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
