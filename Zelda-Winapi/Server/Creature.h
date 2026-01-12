#pragma once
#include "GameObject.h"

class Creature : public GameObject
{
public:
	Creature();
	virtual ~Creature();

	virtual void OnDamaged(CreatureRef attacker);
};

