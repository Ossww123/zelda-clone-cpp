#pragma once
#include "FlipbookActor.h"

class BoxCollider;

enum class PlayerState
{
	Idle ,
	Move ,
	Skill
};

class Player : public FlipbookActor
{
	using Super = FlipbookActor;

public:
	Player ( );
	virtual ~Player ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:

	virtual void TickIdle ( );
	virtual void TickMove ( );
	virtual void TickSkill ( );

	void SetState ( PlayerState state );
	void SetDir ( Dir dir );

	void UpdateAnimation ( );

	bool HasReachedDest ( );
	bool CanGo ( Vec2Int cellPos );
	void SetCellPos ( Vec2Int cellPos , bool teleport = false );

private:
	Flipbook* _flipbookIdle[ 4 ] = {};
	Flipbook* _flipbookMove[ 4 ] = {};
	Flipbook* _flipbookAttack[ 4 ] = {};

	Vec2Int _cellPos = {};
	Vec2 _speed = {};
	Dir _dir = DIR_DOWN;
	PlayerState _state = PlayerState::Idle;
	bool _keyPressed = false;
};

