#pragma once
#include "GameObject.h"

class Projectile : public GameObject
{
	using Super = GameObject;

public:
	Projectile();
	virtual ~Projectile() override;

	virtual void Update() override;

	void SetOwner(uint64 ownerId) { _ownerId = ownerId; }
	uint64 GetOwner() const { return _ownerId; }

	void SetExpireTick(uint64 tick) { _expireTick = tick; }
	uint64 GetExpireTick() const { return _expireTick; }

protected:
	uint64 _ownerId = 0;
	uint64 _expireTick = 0;
	uint64 _waitUntil = 0;
};

