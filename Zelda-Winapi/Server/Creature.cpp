#include "pch.h"
#include "Creature.h"
#include "GameRoom.h"

Creature::Creature()
{
}

Creature::~Creature()
{
}

bool Creature::OnDamaged(CreatureRef attacker, int32& outDamage, float damageMultiplier)
{
	outDamage = 0;
	if (attacker == nullptr)
		return false;

	Protocol::ObjectInfo& attackerInfo = attacker->info;
	Protocol::ObjectInfo& info = this->info;

	int32 baseDamage = attackerInfo.attack() - info.defence();
	if (baseDamage <= 0)
		return false;

	int32 damage = static_cast<int32>(baseDamage * damageMultiplier);

	if (damage <= 0)
		return false;

	int32 newHp = max(0, info.hp() - damage);
	info.set_hp(newHp);

	outDamage = damage;
	return true;
}