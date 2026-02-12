#pragma once
#include "Creature.h"

struct LevelData;
struct PlayerSaveData;

struct InventorySlot
{
	int32 itemId = 0;   // 0 = 빈 슬롯
	int32 count = 0;
};

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

	// 인벤토리
	static const int32 INVENTORY_SIZE = 27;
	int32 FindEmptySlot() const;
	bool AddItem(int32 itemId, int32 count = 1);
	void EquipItem(int32 slot);
	void UnequipItem(int32 equipType);  // 0=weapon, 1=armor, 2=potion
	void UseItem(int32 slot);
	void RecalcStats();
	void SendInventoryData();
	void ApplyFromSaveData(const PlayerSaveData& data);
	PlayerSaveData ToSaveData() const;

public:
	GameSessionRef session;

private:
	void ProcessLevelUp();
	void ApplyLevelStats(const LevelData& data);

	int32 _level = 1;
	int32 _exp = 0;

	InventorySlot _storage[INVENTORY_SIZE];
	InventorySlot _equipWeapon;
	InventorySlot _equipArmor;
	InventorySlot _equipPotion;
};

