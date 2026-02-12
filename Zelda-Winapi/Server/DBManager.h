#pragma once
#include "Player.h"

struct PlayerSaveData
{
	string name;
	int32 level = 1;
	int32 exp = 0;
	int32 hp = 100;
	InventorySlot storage[Player::INVENTORY_SIZE];
	InventorySlot equipWeapon;
	InventorySlot equipArmor;
	InventorySlot equipPotion;
};

struct sqlite3;

class DBManager
{
public:
	static DBManager& GetInstance();
	bool Init(const string& dbPath);
	void Close();

	int64 FindOrCreateAccount(const string& username);
	bool HasPlayerData(int64 accountId);
	bool LoadPlayerData(int64 accountId, PlayerSaveData& outData);
	bool SavePlayerData(int64 accountId, const PlayerSaveData& data);

private:
	void CreateTables();
	sqlite3* _db = nullptr;
};

#define GDBManager DBManager::GetInstance()
