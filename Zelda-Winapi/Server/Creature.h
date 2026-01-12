#pragma once
#include "GameObject.h"

class Creature : public GameObject
{
public:
	Creature();
	virtual ~Creature();

	virtual bool OnDamaged(CreatureRef attacker, int32& outDamage);
};

