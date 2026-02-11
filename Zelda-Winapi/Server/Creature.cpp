#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"

Creature::Creature()
{
}

Creature::~Creature()
{
}

bool Creature::OnDamaged(int32 damage)
{
	if (damage <= 0)
		return false;

	int32 newHp = max(0, info.hp() - damage);
	info.set_hp(newHp);

	return true;
}