#include "pch.h"
#include "GameObject.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "CameraComponent.h"
#include "BoxCollider.h"
#include "SceneManager.h"
#include "DevScene.h"

GameObject::GameObject ( )
{

}

GameObject::~GameObject ( )
{
}

void GameObject::BeginPlay ( )
{
	Super::BeginPlay ( );

	SetState ( MOVE );
	SetState ( IDLE );
	SetAnimState ( IDLE );
	UpdateAnimation ( );
}

void GameObject::Tick ( )
{
	Super::Tick ( );

	float now = GET_SINGLE ( TimeManager )->GetTime ( );

	if ( ( _destPos - _pos ).Length ( ) > 0.5f )
	{
		TickMove ( );

		// 이동 중인데 S_Move에서 IDLE가 와도 MOVE 애니 유지
		if ( IsSkillPlaying ( now ) ) SetAnimState ( SKILL );
		else SetAnimState ( MOVE );
		return;
	}

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

	// 이동 끝났을 때 or 스킬 끝났을 때 IDLE로 복귀
	if ( IsSkillPlaying ( now ) ) SetAnimState ( SKILL );
	else SetAnimState ( IDLE );
}

void GameObject::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void GameObject::SetState ( ObjectState state )
{
	if ( info.state ( ) == state )
		return;

	info.set_state(state);
}

void GameObject::SetDir ( Dir dir )
{
	info.set_dir ( dir );
	UpdateAnimation ( );
}

void GameObject::SetAnimState ( ObjectState state )
{
	if ( _animState == state )
		return;

	_animState = state;
	UpdateAnimation ( );
}

void GameObject::StartSkillAnim ( float duration )
{
	float now = GET_SINGLE ( TimeManager )->GetTime ( );
	_skillAnimEndTime = now + duration;
}

bool GameObject::HasReachedDest ( )
{
	Vec2 dir = ( _destPos - _pos );
	return ( dir.Length ( ) < 10.f );
}

bool GameObject::CanGo ( Vec2Int cellPos )
{
	DevScene* scene = dynamic_cast< DevScene* >( GET_SINGLE ( SceneManager )->GetCurrentScene ( ) );
	if ( scene == nullptr )
		return false;

	return scene->CanGo ( cellPos );
}

Dir GameObject::GetLookAtDir ( Vec2Int cellPos )
{
	Vec2Int dir = cellPos - GetCellPos ( );
	if ( dir.x > 0 )
		return DIR_RIGHT;
	else if ( dir.x < 0 )
		return DIR_LEFT;
	else if ( dir.y > 0 )
		return DIR_DOWN;
	else
		return DIR_UP;
}

void GameObject::SetCellPos ( Vec2Int cellPos , bool teleport )
{
	info.set_posx ( cellPos.x );
	info.set_posy ( cellPos.y );

	DevScene* scene = dynamic_cast< DevScene* >( GET_SINGLE ( SceneManager )->GetCurrentScene ( ) );
	if ( scene == nullptr )
		return;

	_destPos = scene->ConvertPos ( cellPos );

	if ( teleport )
		_pos = _destPos;
}

Vec2Int GameObject::GetCellPos ( )
{
	return Vec2Int{ info.posx ( ), info.posy ( ) };
}

Vec2Int GameObject::GetFrontCellPos ( )
{
	switch ( info.dir() )
	{
	case DIR_UP:
		return GetCellPos() + Vec2Int{0, -1};
	case DIR_DOWN:
		return GetCellPos ( ) + Vec2Int{ 0, 1 };
	case DIR_LEFT:
		return GetCellPos ( ) + Vec2Int{ -1, 0 };
	case DIR_RIGHT:
		return GetCellPos ( ) + Vec2Int{ 1, 0 };
	}

	return GetCellPos ( );
}
