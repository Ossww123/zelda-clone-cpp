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
	int32 GetAggroRange() const { return _aggroRange; }
	int32 GetLeashRange() const { return _leashRange; }

	void SetTemplateId(int32 id) { _templateId = id; info.mutable_monster()->set_templateid(id); }
	int32 GetTemplateId() const { return _templateId; }

private:
	virtual void UpdateIdle();
	virtual void UpdateMove();
	virtual void UpdateSkill();

private:
	uint64 _waitUntil = 0;
	weak_ptr<Player> _target;

	Vec2Int _homePos = { -1, -1 };

	int32 _aggroRange = 8;  // 어그로 범위
	int32 _leashRange = 10; // 리시 범위
	int32 _templateId = 1001; // 기본값: Snake
};

