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

	// SyncToServer ( );
}

void MyPlayer::Render ( HDC hdc )
{
	Super::Render (hdc );
}

void MyPlayer::TickInput ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	_keyPressed = true;
	Vec2Int deltaXY[ 4 ] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::W ) )
	{
		SetDir ( DIR_UP );
	}
	else  if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::S ) )
	{
		SetDir ( DIR_DOWN );
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::A ) )
	{
		SetDir ( DIR_LEFT );
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::D ) )
	{
		SetDir ( DIR_RIGHT );
	}
	else
	{
		_keyPressed = false;
	}

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::KEY_1 ) )
	{
		SetWeaponType ( WeaponType::Sword );
	}
	else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::KEY_2 ) )
	{
		SetWeaponType ( WeaponType::Bow );
	}
	else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::KEY_3 ) )
	{
		SetWeaponType ( WeaponType::Staff );
	}

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::SpaceBar ) )
	{
		SetState ( SKILL );
		TrySkill ( );
	}
}

void MyPlayer::TryMove ( )
{
	if ( _keyPressed == false )
		return;

	Vec2Int deltaXY[ 4 ] = { {0,-1}, {0,1}, {-1,0}, {1,0} };

	Vec2Int nextPos = GetCellPos ( ) + deltaXY[ info.dir ( ) ];
	if ( CanGo ( nextPos ) == false )
		return;

	SetCellPos ( nextPos );
	SetState ( MOVE );

	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Move ( info.dir ( ) , nextPos.x , nextPos.y );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
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

void MyPlayer::SyncToServer ( )
{
	//SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Move ( );
	//GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}
