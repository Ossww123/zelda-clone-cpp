#pragma once
#include "pch.h"
#include "RoomConfig.h"

class RoomDataManager
{
public:
	static RoomDataManager& GetInstance()
	{
		static RoomDataManager instance;
		return instance;
	}

	bool LoadAllData();
	bool LoadRoomConfigs(const string& csvPath);
	bool LoadMonsterTemplates(const string& csvPath);
	bool LoadMonsterSpawns(const string& jsonPath);

	const RoomConfigData* GetRoomConfig(const string& roomId) const;
	const RoomSpawnConfig* GetSpawnConfig(const string& roomId) const;
	const MonsterTemplateData* GetMonsterTemplate(int32 templateId) const;

	vector<string> GetStaticRoomIds() const;
	vector<string> GetInstanceRoomIds() const;

private:
	RoomDataManager() = default;
	~RoomDataManager() = default;
	RoomDataManager(const RoomDataManager&) = delete;
	RoomDataManager& operator=(const RoomDataManager&) = delete;

	unordered_map<string, RoomConfigData> _roomConfigs;
	unordered_map<string, RoomSpawnConfig> _spawnConfigs;
	unordered_map<int32, MonsterTemplateData> _monsterTemplates;

	// JSON 파싱 헬퍼
	bool ParseMonsterSpawnJson(const string& jsonContent);

	// JSON 유틸리티
	size_t FindKey(const string& json, const string& key, size_t startPos);
	string GetStringValue(const string& json, const string& key, size_t startPos);
	int32 GetIntValue(const string& json, const string& key, size_t startPos);
	vector<int32> GetIntArray(const string& json, const string& key, size_t startPos);
	string GetObjectBlock(const string& json, size_t startPos, size_t& outEndPos);
	vector<string> GetObjectsInArray(const string& json, const string& arrayKey, size_t startPos);

	bool ParseBool(const string& value);
	wstring StringToWString(const string& str);
};

#define GRoomDataManager RoomDataManager::GetInstance()
