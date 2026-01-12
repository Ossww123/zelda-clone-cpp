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

protected:

	virtual void TickIdle ( ) override;
	virtual void TickMove ( ) override;
	virtual void TickSkill ( ) override;
	virtual void UpdateAnimation ( ) override;

public:
	void SetWeaponType ( WeaponType weaponType ) { _weaponType = weaponType; }
	WeaponType GetWeaponType ( ) { return _weaponType; }

	static Protocol::WEAPON_TYPE ToProtoWeaponType ( WeaponType wt )
	{
		switch ( wt )
		{
		case WeaponType::Sword: return Protocol::WEAPON_TYPE_SWORD;
		case WeaponType::Bow:   return Protocol::WEAPON_TYPE_BOW;
		case WeaponType::Staff: return Protocol::WEAPON_TYPE_STAFF;
		}
		return Protocol::WEAPON_TYPE_SWORD;
	}
	static WeaponType FromProtoWeaponType ( Protocol::WEAPON_TYPE wt )
	{
		switch ( wt )
		{
		case Protocol::WEAPON_TYPE_SWORD: return WeaponType::Sword;
		case Protocol::WEAPON_TYPE_BOW:   return WeaponType::Bow;
		case Protocol::WEAPON_TYPE_STAFF: return WeaponType::Staff;
		default:                          return WeaponType::Sword;
		}
	}

private:
	Flipbook* _flipbookIdle[ 4 ] = {};
	Flipbook* _flipbookMove[ 4 ] = {};
	Flipbook* _flipbookAttack[ 4 ] = {};
	Flipbook* _flipbookBow[ 4 ] = {};
	Flipbook* _flipbookStaff[ 4 ] = {};

	bool _keyPressed = false;
	WeaponType _weaponType = WeaponType::Sword;
};

