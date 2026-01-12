#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"

Creature::Creature()
{
}

Creature::~Creature()
{
}

void Creature::OnDamaged(CreatureRef attacker)
{
	if (attacker == nullptr)
		return;

	Protocol::ObjectInfo& attackerInfo = attacker->info;
	Protocol::ObjectInfo& info = this->info;

	int32 damage = attackerInfo.attack() - info.defence();
	if (damage <= 0)
		return;

	info.set_hp(max(0, info.hp() - damage));

	if (info.hp() == 0)
	{
		GRoom->RemoveObject(info.objectid());
	}
}