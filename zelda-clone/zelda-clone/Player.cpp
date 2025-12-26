#include "pch.h"
#include "Player.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "CameraComponent.h"
#include "BoxCollider.h"

Player::Player ( )
{
	_flipbookUp		= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveUp" );
	_flipbookDown	= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveDown" );
	_flipbookLeft	= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveLeft" );
	_flipbookRight	= GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_MoveRight" );

	CameraComponent* camera = new CameraComponent ( );
	AddComponent ( camera );
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

void Player::OnComponentBeginOverlap ( Collider* collider , Collider* other )
{
	BoxCollider* b1 = dynamic_cast< BoxCollider* >( collider );
	BoxCollider* b2 = dynamic_cast< BoxCollider* >( other );
	if ( b1 == nullptr || b2 == nullptr )
		return;

	AdjustCollisionPos ( b1 , b2 );
}

void Player::OnComponentEndOverlap ( Collider* collider , Collider* other )
{
}

void Player::TickGravity ( )
{
}

void Player::AdjustCollisionPos ( BoxCollider* b1 , BoxCollider* b2 )
{
	RECT r1 = b1->GetRect ( );
	RECT r2 = b2->GetRect ( );

	Vec2 pos = GetPos ( );

	RECT intersect = {};
	if ( ::IntersectRect ( &intersect , &r1 , &r2 ))
	{
		int32 w = intersect.right - intersect.left;
		int32 h = intersect.bottom - intersect.top;

		if ( w > h )
		{
			if ( intersect.top == r2.top )
			{
				pos.y -= h;
			}
			else
			{
				pos.y += h;
			}
		}
		else
		{
			if ( intersect.left == r2.left )
			{
				pos.x -= w;
			}
			else
			{
				pos.x += w;
			}
		}
	}

	SetPos ( pos );
}
