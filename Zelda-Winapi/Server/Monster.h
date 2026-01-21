#pragma once
#include "Creature.h"

class Monster : public Creature
{
	using Super = Creature;

public:
	Monster();
	virtual ~Monster() override;

	virtual void Update();

public:
	void SetHomePos(Vec2Int pos) { _homePos = pos; }
	Vec2Int GetHomePos() const { return _homePos; }

	void SetAggroRange(int32 r) { _aggroRange = r; }
	void SetLeashRange(int32 r) { _leashRange = r; }

private:
	virtual void UpdateIdle();
	virtual void UpdateMove();
	virtual void UpdateSkill();

private:
	uint64 _waitUntil = 0;
	weak_ptr<Player> _target;

	Vec2Int _homePos = { -1, -1 };

	int32 _aggroRange = 8;  // 감지 범위
	int32 _leashRange = 10; // 추적 범위
};

