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
};
