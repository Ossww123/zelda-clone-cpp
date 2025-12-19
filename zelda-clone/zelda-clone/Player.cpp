#include "pch.h"
#include "Player.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"

Player::Player ( )
{
	_flipbookUp		= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveUp" );
	_flipbookDown	= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveDown" );
	_flipbookLeft	= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveLeft" );
	_flipbookRight	= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveRight" );
}

Player::~Player ( )
{
}

void Player::BeginPlay ( )
{
	Super::BeginPlay ( );

	SetFlipbook ( _flipbookRight );
}

void Player::Tick ( )
{
	Super::Tick ( );

	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	// TODO
	if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::W ) )
	{
		_pos.y -= 200 * deltaTime;
		SetFlipbook ( _flipbookUp );
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::S ) )
	{
		_pos.y += 200 * deltaTime;
		SetFlipbook ( _flipbookDown );
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::A ) )
	{
		_pos.x -= 200 * deltaTime;
		SetFlipbook ( _flipbookLeft );
	}
	else if ( GET_SINGLE ( InputManager )->GetButton ( KeyType::D ) )
	{
		_pos.x += 200 * deltaTime;
		SetFlipbook ( _flipbookRight );
	}
}

void Player::Render ( HDC hdc )
{
	Super::Render ( hdc );
}
