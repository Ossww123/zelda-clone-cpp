#pragma once

#include "FlipbookActor.h"

class GameObject : public FlipbookActor
{
	using Super = FlipbookActor;

public:
	GameObject ( );
	virtual ~GameObject ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

protected:

	virtual void TickIdle ( ) {};
	virtual void TickMove ( ) {};
	virtual void TickSkill ( ) {};
	virtual void UpdateAnimation ( ) {};


public:
	void SetState ( ObjectState state );
	void SetDir ( Dir dir );
	void SetAnimState ( ObjectState state );
	ObjectState GetAnimState ( ) const { return _animState; }
	void StartSkillAnim ( float duration );
	bool IsSkillPlaying ( float now ) const { return now < _skillAnimEndTime; }

	bool HasReachedDest ( );
	bool CanGo ( Vec2Int cellPos );
	Dir GetLookAtDir ( Vec2Int cellPos );

	void SetCellPos ( Vec2Int cellPos , bool teleport = false );
	Vec2Int GetCellPos ( );
	Vec2Int GetFrontCellPos ( );

	int64 GetObjectID ( ) { return info.objectid ( ); }
	void SetObjectID ( int64 id ) { info.set_objectid ( id ); }

public:
	Protocol::ObjectInfo info;

protected:
	ObjectState _animState = IDLE;
	float _skillAnimEndTime = 0.f;
};

