#include "pch.h"
#include "Player.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "ObjectManager.h"

#include "Missile.h"

Player::Player ( ) : Object(ObjectType::Player )
{
}

Player::~Player ( )
{
}

void Player::Init ( )
{
	// TODO : Data
	_stat.hp = 100;
	_stat.maxHp = 100;
	_stat.speed = 500;

	_pos.x = 400;
	_pos.y = 500;
}

void Player::Update ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::A ) )
	{
		_pos.x -= _stat.speed * deltaTime;
	}

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::D ) )
	{
		_pos.x += _stat.speed * deltaTime;
	}

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::W ) )
	{
		_pos.y -= _stat.speed * deltaTime;
	}

	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::S ) )
	{
		_pos.y += _stat.speed * deltaTime;
	}

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::SpaceBar ) )
	{
		// TODO : 미사일 발사
		Missile* missile = GET_SINGLE ( ObjectManager )->CreateObject<Missile> ( );
		missile->SetPos ( _pos );
		GET_SINGLE ( ObjectManager )->Add ( missile );
	}
}

void Player::Render ( HDC hdc )
{
	Utils::DrawCircle ( hdc , _pos , 50 );
}
