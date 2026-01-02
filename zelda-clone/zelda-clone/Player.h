#pragma once
#include "Creature.h"

class Player : public Creature
{
	using Super = Creature;

public:
	Player ( );
	virtual ~Player ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:

	virtual void TickIdle ( ) override;
	virtual void TickMove ( ) override;
	virtual void TickSkill ( ) override;
	void UpdateAnimation ( ) override;

private:
	Flipbook* _flipbookIdle[ 4 ] = {};
	Flipbook* _flipbookMove[ 4 ] = {};
	Flipbook* _flipbookAttack[ 4 ] = {};

	bool _keyPressed = false;
};

