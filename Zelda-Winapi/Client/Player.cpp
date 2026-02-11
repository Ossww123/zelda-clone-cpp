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
#include "HitEffect.h"
#include "Arrow.h"

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

	_flipbookBow[ DIR_UP ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_BowUp" );
	_flipbookBow[ DIR_DOWN ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_BowDown" );
	_flipbookBow[ DIR_LEFT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_BowLeft" );
	_flipbookBow[ DIR_RIGHT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_BowRight" );

	_flipbookStaff[ DIR_UP ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_StaffUp" );
	_flipbookStaff[ DIR_DOWN ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_StaffDown" );
	_flipbookStaff[ DIR_LEFT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_StaffLeft" );
	_flipbookStaff[ DIR_RIGHT ] = GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_StaffRight" );

}

Player::~Player ( )
{
}

void Player::BeginPlay ( )
{
	Super::BeginPlay ( );

	SetState ( MOVE );
	SetState ( IDLE );
}

void Player::Tick ( )
{
	Super::Tick ( );

	switch ( info.state() )
	{
	case IDLE:
		TickIdle ( );
		break;
	case MOVE:
		TickMove ( );
		break;
	case SKILL:
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
	if ( ( _destPos - _pos ).Length ( ) >= 1.f )
		TickMove ( );
}

void Player::TickMove ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	const float speed = 240.f; // 기존 값
	Vec2 toDest = _destPos - _pos;
	float dist = toDest.Length ( );

	if ( dist <= 0.5f )
	{
		_pos = _destPos;
		return;
	}

	float step = speed * deltaTime;
	if ( step >= dist )
	{
		_pos = _destPos;
		return;
	}

	Vec2 dir = toDest / dist;
	_pos += dir * step;

	//Vec2 dir = ( _destPos - _pos );
	//if ( dir.Length ( ) < 1.f )
	//{
	//	SetState ( IDLE );
	//	_pos = _destPos;
	//}
	//else
	//{
	//	switch (info.dir())
	//	{
	//	case DIR_UP:
	//		_pos.y -= 100 * deltaTime;
	//		break;
	//	case DIR_DOWN:
	//		_pos.y += 100 * deltaTime;
	//		break;
	//	case DIR_LEFT:
	//		_pos.x -= 100 * deltaTime;
	//		break;
	//	case DIR_RIGHT:
	//		_pos.x += 100 * deltaTime;
	//		break;
	//	}
	//}
}

void Player::TickSkill ( )
{
	if ( _flipbook == nullptr )
		return;

	if ( IsAnimationEnded ( ) )
	{
		SetState ( IDLE );
	}
}

void Player::UpdateAnimation ( )
{
	switch ( info.state() )
	{
	case IDLE:
		SetFlipbook ( _flipbookIdle[info.dir()] );
		break;
	case MOVE:
		SetFlipbook ( _flipbookMove[info.dir()] );
		break;
	case SKILL:
		if (_weaponType == WeaponType::Sword )
			SetFlipbook ( _flipbookAttack[info.dir()] );
		else if ( _weaponType == WeaponType::Bow )
			SetFlipbook ( _flipbookBow[info.dir()] );
		else if ( _weaponType == WeaponType::Staff )
			SetFlipbook ( _flipbookStaff[info.dir()] );
		break;
	}
}
