#pragma once
#include "pch.h"

struct RoomConfigData
{
	string roomId;
	string roomName;
	bool skillEnabled;
	bool monsterSpawnEnabled;
	uint32 respawnTimeMs;
	wstring tilemapPath;
	int32 maxPlayers;
	bool isInstance;
};

struct MonsterSpawnData
{
	string groupId;
	Vec2Int anchor;
	vector<Vec2Int> offsets;

	struct MonsterInfo
	{
		int32 templateId;
		int32 count;
		int32 level;
		int32 aggroRange;
		int32 leashRange;
	};

	vector<MonsterInfo> monsters;
};

struct RoomSpawnConfig
{
	string roomId;
	vector<MonsterSpawnData> spawns;
};

struct MonsterTemplateData
{
	int32 templateId;
	string name;
	int32 maxHp;
	int32 attack;
	int32 defence;
	int32 exp;
};

struct LevelData
{
	int32 level;
	int32 requiredExp;
	int32 maxHp;
	int32 attack;
	int32 defence;
};

struct ItemTemplateData
{
	int32 itemId;
	string name;
	string type;       // "consumable", "weapon", "armor"
	int32 value;
	int32 maxStack;
};

struct MonsterDropData
{
	int32 templateId;
	int32 itemId;
	int32 dropRate;    // 0~100
};
