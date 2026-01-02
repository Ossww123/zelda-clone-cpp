#include "pch.h"
#include "Player.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "CameraComponent.h"
#include "BoxCollider.h"
#include "SceneManager.h"
#include "DevScene.h"

Player::Player ( )
{
	_flipbookIdle[ DIR_UP ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_IdleUp" );
	_flipbookIdle[ DIR_DOWN ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_IdleDown" );
	_flipbookIdle[ DIR_LEFT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_IdleLeft" );
	_flipbookIdle[ DIR_RIGHT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_IdleRight" );

	_flipbookMove[ DIR_UP ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveUp" );
	_flipbookMove[ DIR_DOWN ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveDown" );
	_flipbookMove[ DIR_LEFT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveLeft" );
	_flipbookMove[ DIR_RIGHT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveRight" );

	_flipbookAttack[ DIR_UP ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_AttackUp" );
	_flipbookAttack[ DIR_DOWN ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_AttackDown" );
	_flipbookAttack[ DIR_LEFT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_AttackLeft" );
	_flipbookAttack[ DIR_RIGHT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_AttackRight" );

	CameraComponent* camera = new CameraComponent ( );
	AddComponent ( camera );
}

Player::~Player ( )
{
}

void Player::BeginPlay ( )
{
	Super::BeginPlay ( );

	SetState ( ObjectState::Move );
	SetState ( ObjectState::Idle );

	SetCellPos ( { 5, 5 } , true );
}

void Player::Tick ( )
{
	Super::Tick ( );

	switch ( _state )
	{
	case ObjectState::Idle:
		TickIdle ( );
		break;
	case ObjectState::Move:
		TickMove ( );
		break;
	case ObjectState::Skill:
		TickSkill ( );
		break;
	}
}

void Player::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void Player::TickIdle ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	_keyPressed = true;
	Vec2Int deltaXY[ 4 ] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::W ) )
	{
		SetDir ( DIR_UP );

		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( ObjectState::Move );
		}
	}
	else  if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::S ) )
	{
		SetDir ( DIR_DOWN );

		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( ObjectState::Move );
		}
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::A ) )
	{
		SetDir ( DIR_LEFT );
		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( ObjectState::Move );
		}
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::D ) )
	{
		SetDir ( DIR_RIGHT );
		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( ObjectState::Move );
		}
	}
	else
	{
		_keyPressed = false;
		if ( _state == ObjectState::Idle )
			UpdateAnimation ( );
	}
}

void Player::TickMove ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	Vec2 dir = ( _destPos - _pos );
	if ( dir.Length ( ) < 5.f )
	{
		SetState ( ObjectState::Idle );
		_pos = _destPos;
	}
	else
	{
		switch ( _dir )
		{
		case DIR_UP:
			_pos.y -= 200 * deltaTime;
			break;
		case DIR_DOWN:
			_pos.y += 200 * deltaTime;
			break;
		case DIR_LEFT:
			_pos.x -= 200 * deltaTime;
			break;
		case DIR_RIGHT:
			_pos.x += 200 * deltaTime;
			break;
		}
	}
}

void Player::TickSkill ( )
{

}

void Player::UpdateAnimation ( )
{
	switch ( _state )
	{
	case ObjectState::Idle:
		if ( _keyPressed )
			SetFlipbook ( _flipbookMove[ _dir ] );
		else
			SetFlipbook ( _flipbookIdle[ _dir ] );
		break;
	case ObjectState::Move:
		SetFlipbook ( _flipbookMove[ _dir ] );
		break;
	case ObjectState::Skill:
		SetFlipbook ( _flipbookAttack[ _dir ] );
		break;
	}
}
