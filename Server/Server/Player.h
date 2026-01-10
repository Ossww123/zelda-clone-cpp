#pragma once
#include "Creature.h"

class Player : public Creature
{
	using Super = Creature;

public:
	Player();
	virtual ~Player();

	virtual void Update();

public:
	GameSessionRef session;
};

