#pragma once
#include "Projectile.h"

class Arrow : public Projectile
{
	using Super = Projectile;

public:
	Arrow();
	virtual ~Arrow() override;

	virtual void Update() override;

private:
	void UpdateIdle();
	void UpdateMove();

private:
	uint64 _moveIntervalMs = 80;
	uint64 _lifeTimeMs = 1500;
	bool _spawned = false;
};

