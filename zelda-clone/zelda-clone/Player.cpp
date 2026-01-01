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

	SetState ( PlayerState::Move );
	SetState ( PlayerState::Idle );

	SetCellPos ( { 5, 5 } , true );
}

void Player::Tick ( )
{
	Super::Tick ( );

	switch ( _state )
	{
	case PlayerState::Idle:
		TickIdle ( );
		break;
	case PlayerState::Move:
		TickMove ( );
		break;
	case PlayerState::Skill:
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
			SetState ( PlayerState::Move );
		}
	}
	else  if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::S ) )
	{
		SetDir ( DIR_DOWN );

		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( PlayerState::Move );
		}
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::A ) )
	{
		SetDir ( DIR_LEFT );
		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( PlayerState::Move );
		}
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::D ) )
	{
		SetDir ( DIR_RIGHT );
		Vec2Int nextPos = _cellPos + deltaXY[ _dir ];
		if ( CanGo ( nextPos ) )
		{
			SetCellPos ( nextPos );
			SetState ( PlayerState::Move );
		}
	}
	else
	{
		_keyPressed = false;
		if ( _state == PlayerState::Idle )
			UpdateAnimation ( );
	}
}

void Player::TickMove ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	Vec2 dir = ( _destPos - _pos );
	if ( dir.Length ( ) < 10.f )
	{
		SetState ( PlayerState::Idle );
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

void Player::SetState ( PlayerState state )
{
	if ( _state == state )
		return;

	_state = state;
	UpdateAnimation ( );
}

void Player::SetDir ( Dir dir )
{
	_dir = dir;
	UpdateAnimation ( );
}

void Player::UpdateAnimation ( )
{
	switch ( _state )
	{
	case PlayerState::Idle:
		if ( _keyPressed )
			SetFlipbook ( _flipbookMove[ _dir ] );
		else
			SetFlipbook ( _flipbookIdle[ _dir ] );
		break;
	case PlayerState::Move:
		SetFlipbook ( _flipbookMove[ _dir ] );
		break;
	case PlayerState::Skill:
		SetFlipbook ( _flipbookAttack[ _dir ] );
		break;
	}
}

bool Player::HasReachedDest ( )
{
	Vec2 dir = ( _destPos - _pos );
	return ( dir.Length ( ) < 10.f );
}

bool Player::CanGo ( Vec2Int cellPos )
{
	DevScene* scene = dynamic_cast< DevScene* >( GET_SINGLE ( SceneManager )->GetCurrentScene ( ) );
	if ( scene == nullptr )
		return false;

	return scene->CanGo ( cellPos );
}

void Player::SetCellPos ( Vec2Int cellPos , bool teleport )
{
	_cellPos = cellPos;

	DevScene* scene = dynamic_cast< DevScene* >( GET_SINGLE ( SceneManager )->GetCurrentScene ( ) );
	if ( scene == nullptr )
		return;

	_destPos = scene->ConvertPos ( cellPos );

	if ( teleport )
		_pos = _destPos;
}
