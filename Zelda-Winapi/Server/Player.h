#pragma once
#include "Creature.h"

struct LevelData;

class Player : public Creature
{
	using Super = Creature;

public:
	Player();
	virtual ~Player();

	virtual void Update();

	void GainExp(int32 amount);
	int32 GetLevel() const { return _level; }
	int32 GetExp() const { return _exp; }
	int32 GetMaxExp() const;

public:
	GameSessionRef session;

private:
	void ProcessLevelUp();
	void ApplyLevelStats(const LevelData& data);

	int32 _level = 1;
	int32 _exp = 0;
};

