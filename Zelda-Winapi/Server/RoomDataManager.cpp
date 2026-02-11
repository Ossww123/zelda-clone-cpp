#include "pch.h"
#include "RoomDataManager.h"
#include <fstream>
#include <sstream>

bool RoomDataManager::LoadAllData()
{
	bool success = true;

	success &= LoadRoomConfigs("../Datasheets/RoomConfig.csv");
	success &= LoadMonsterTemplates("../Datasheets/MonsterTemplate.csv");
	success &= LoadMonsterSpawns("../Datasheets/MonsterSpawn.json");
	success &= LoadLevelData("../Datasheets/LevelData.csv");
	success &= LoadItemTemplates("../Datasheets/ItemTemplate.csv");
	success &= LoadMonsterDrops("../Datasheets/MonsterDrop.csv");

	if (success)
	{
		cout << "[RoomDataManager] Successfully loaded all data:" << endl;
		cout << "  - Room Configs: " << _roomConfigs.size() << endl;
		cout << "  - Monster Templates: " << _monsterTemplates.size() << endl;
		cout << "  - Spawn Configs: " << _spawnConfigs.size() << endl;
		cout << "  - Level Data: " << _levelData.size() << endl;
		cout << "  - Item Templates: " << _itemTemplates.size() << endl;
		cout << "  - Monster Drops: " << _monsterDrops.size() << " monster types" << endl;
	}
	else
	{
		cout << "[RoomDataManager] ERROR: Failed to load some data files!" << endl;
	}

	return success;
}

bool RoomDataManager::LoadRoomConfigs(const string& csvPath)
{
	ifstream file(csvPath);
	if (!file.is_open())
	{
		cout << "[RoomDataManager] ERROR: Failed to open " << csvPath << endl;
		return false;
	}

	string line;
	getline(file, line); // 헤더 스킵

	while (getline(file, line))
	{
		if (line.empty()) continue;

		stringstream ss(line);
		string token;
		RoomConfigData data;

		// RoomId
		getline(ss, data.roomId, ',');

		// RoomName
		getline(ss, data.roomName, ',');

		// SkillEnabled
		getline(ss, token, ',');
		data.skillEnabled = ParseBool(token);

		// MonsterSpawnEnabled
		getline(ss, token, ',');
		data.monsterSpawnEnabled = ParseBool(token);

		// RespawnTimeMs
		getline(ss, token, ',');
		data.respawnTimeMs = static_cast<uint32>(stoi(token));

		// TilemapPath
		getline(ss, token, ',');
		data.tilemapPath = StringToWString(token);

		// MaxPlayers
		getline(ss, token, ',');
		data.maxPlayers = stoi(token);

		// IsInstance
		getline(ss, token, ',');
		data.isInstance = ParseBool(token);

		_roomConfigs[data.roomId] = data;
	}

	cout << "[RoomDataManager] Loaded " << _roomConfigs.size() << " room configs" << endl;
	return true;
}

bool RoomDataManager::LoadMonsterTemplates(const string& csvPath)
{
	ifstream file(csvPath);
	if (!file.is_open())
	{
		cout << "[RoomDataManager] ERROR: Failed to open " << csvPath << endl;
		return false;
	}

	string line;
	getline(file, line); // 헤더 스킵

	while (getline(file, line))
	{
		if (line.empty()) continue;

		stringstream ss(line);
		string token;
		MonsterTemplateData data;

		// templateId
		getline(ss, token, ',');
		data.templateId = stoi(token);

		// name
		getline(ss, data.name, ',');

		// maxHp
		getline(ss, token, ',');
		data.maxHp = stoi(token);

		// attack
		getline(ss, token, ',');
		data.attack = stoi(token);

		// defence
		getline(ss, token, ',');
		data.defence = stoi(token);

		// exp
		getline(ss, token, ',');
		data.exp = stoi(token);

		_monsterTemplates[data.templateId] = data;
	}

	cout << "[RoomDataManager] Loaded " << _monsterTemplates.size() << " monster templates" << endl;
	return true;
}

bool RoomDataManager::LoadMonsterSpawns(const string& jsonPath)
{
	ifstream file(jsonPath);
	if (!file.is_open())
	{
		cout << "[RoomDataManager] ERROR: Failed to open " << jsonPath << endl;
		return false;
	}

	stringstream buffer;
	buffer << file.rdbuf();
	string jsonContent = buffer.str();

	bool success = ParseMonsterSpawnJson(jsonContent);

	if (success)
	{
		cout << "[RoomDataManager] Loaded " << _spawnConfigs.size() << " spawn configs" << endl;

		for (const auto& [roomId, config] : _spawnConfigs)
		{
			cout << "  - Room: " << roomId << ", Spawn groups: " << config.spawns.size() << endl;
		}
	}
	else
	{
		cout << "[RoomDataManager] ERROR: Failed to parse MonsterSpawn.json" << endl;
	}

	return success;
}

bool RoomDataManager::ParseMonsterSpawnJson(const string& json)
{
	try
	{
		// 최상위 { } 안에서 각 룸 찾기
		size_t pos = 0;
		while (true)
		{
			// 다음 룸 이름 찾기
			size_t quoteStart = json.find("\"", pos);
			if (quoteStart == string::npos || quoteStart >= json.length() - 1)
				break;

			size_t quoteEnd = json.find("\"", quoteStart + 1);
			if (quoteEnd == string::npos)
				break;

			string roomId = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);

			// "spawns" 키워드가 있는지 확인 (실제 룸 객체인지)
			size_t roomObjStart = json.find("{", quoteEnd);
			if (roomObjStart == string::npos)
				break;

			size_t spawnsKeyPos = json.find("\"spawns\"", roomObjStart);
			if (spawnsKeyPos == string::npos || spawnsKeyPos > json.find("}", roomObjStart))
			{
				pos = quoteEnd + 1;
				continue;
			}

			cout << "[RoomDataManager] Parsing room: " << roomId << endl;

			RoomSpawnConfig config;
			config.roomId = roomId;

			// spawns 배열 안의 모든 spawn group 객체 추출
			vector<string> spawnGroupObjects = GetObjectsInArray(json, "spawns", roomObjStart);
			cout << "[RoomDataManager] Found " << spawnGroupObjects.size() << " spawn groups" << endl;

			for (const string& groupJson : spawnGroupObjects)
			{
				MonsterSpawnData spawnData;

				// groupId
				spawnData.groupId = GetStringValue(groupJson, "groupId", 0);

				// anchor
				vector<int32> anchorArray = GetIntArray(groupJson, "anchor", 0);
				if (anchorArray.size() >= 2)
				{
					spawnData.anchor = Vec2Int{ anchorArray[0], anchorArray[1] };
				}

				// offsets (2차원 배열: [[0,0], [1,0], [0,1]])
				size_t offsetsPos = FindKey(groupJson, "offsets", 0);
				if (offsetsPos != string::npos)
				{
					size_t arrayStart = groupJson.find("[", offsetsPos);
					if (arrayStart != string::npos)
					{
						// 외부 배열의 끝 찾기
						int bracketCount = 1;
						size_t arrayEnd = arrayStart + 1;
						while (arrayEnd < groupJson.length() && bracketCount > 0)
						{
							if (groupJson[arrayEnd] == '[') bracketCount++;
							else if (groupJson[arrayEnd] == ']') bracketCount--;
							arrayEnd++;
						}

						cout << "[RoomDataManager] Parsing offsets array" << endl;

						size_t currentPos = arrayStart + 1;
						while (currentPos < arrayEnd)
						{
							// 다음 [ 찾기 (내부 배열 시작)
							size_t pairStart = groupJson.find("[", currentPos);
							if (pairStart == string::npos || pairStart >= arrayEnd - 1)
								break;

							// 대응하는 ] 찾기
							size_t pairEnd = groupJson.find("]", pairStart);
							if (pairEnd == string::npos || pairEnd >= arrayEnd)
								break;

							// [x, y] 에서 숫자들 추출
							vector<int32> pair;
							size_t numPos = pairStart + 1;
							while (numPos < pairEnd)
							{
								// 공백, 쉼표 건너뛰기
								while (numPos < pairEnd && (groupJson[numPos] == ' ' || groupJson[numPos] == ',' || groupJson[numPos] == '\t' || groupJson[numPos] == '\n'))
									numPos++;

								if (numPos >= pairEnd)
									break;

								// 숫자 읽기
								bool isNegative = false;
								if (groupJson[numPos] == '-')
								{
									isNegative = true;
									numPos++;
								}

								size_t numEnd = numPos;
								while (numEnd < pairEnd && isdigit(groupJson[numEnd]))
									numEnd++;

								if (numEnd > numPos)
								{
									string numStr = groupJson.substr(numPos, numEnd - numPos);
									int32 value = stoi(numStr);
									if (isNegative)
										value = -value;
									pair.push_back(value);
								}

								numPos = numEnd;
							}

							if (pair.size() >= 2)
							{
								spawnData.offsets.push_back(Vec2Int{ pair[0], pair[1] });
								cout << "[RoomDataManager]   - offset: (" << pair[0] << ", " << pair[1] << ")" << endl;
							}

							currentPos = pairEnd + 1;
						}

						cout << "[RoomDataManager] Total offsets parsed: " << spawnData.offsets.size() << endl;
					}
				}

				// monsters 배열
				vector<string> monsterObjects = GetObjectsInArray(groupJson, "monsters", 0);
				cout << "[RoomDataManager] Found " << monsterObjects.size() << " monster types in group" << endl;

				for (const string& monsterJson : monsterObjects)
				{
					MonsterSpawnData::MonsterInfo monsterInfo;

					monsterInfo.templateId = GetIntValue(monsterJson, "templateId", 0);
					monsterInfo.count = GetIntValue(monsterJson, "count", 0);
					monsterInfo.level = GetIntValue(monsterJson, "level", 0);
					monsterInfo.aggroRange = GetIntValue(monsterJson, "aggroRange", 0);
					monsterInfo.leashRange = GetIntValue(monsterJson, "leashRange", 0);

					cout << "[RoomDataManager]   - templateId=" << monsterInfo.templateId
						<< ", count=" << monsterInfo.count << endl;

					spawnData.monsters.push_back(monsterInfo);
				}

				config.spawns.push_back(spawnData);
			}

			_spawnConfigs[roomId] = config;

			// 다음 룸으로
			pos = quoteEnd + 1;
		}

		return true;
	}
	catch (const exception& e)
	{
		cout << "[RoomDataManager] ERROR: Failed to parse MonsterSpawn.json: " << e.what() << endl;
		return false;
	}
	catch (...)
	{
		cout << "[RoomDataManager] ERROR: Failed to parse MonsterSpawn.json (unknown error)" << endl;
		return false;
	}
}

// JSON 파싱 헬퍼 함수들
size_t RoomDataManager::FindKey(const string& json, const string& key, size_t startPos)
{
	string searchKey = "\"" + key + "\"";
	return json.find(searchKey, startPos);
}

string RoomDataManager::GetStringValue(const string& json, const string& key, size_t startPos)
{
	size_t keyPos = FindKey(json, key, startPos);
	if (keyPos == string::npos)
		return "";

	size_t colonPos = json.find(":", keyPos);
	if (colonPos == string::npos)
		return "";

	size_t quoteStart = json.find("\"", colonPos);
	if (quoteStart == string::npos)
		return "";

	size_t quoteEnd = json.find("\"", quoteStart + 1);
	if (quoteEnd == string::npos)
		return "";

	return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

int32 RoomDataManager::GetIntValue(const string& json, const string& key, size_t startPos)
{
	size_t keyPos = FindKey(json, key, startPos);
	if (keyPos == string::npos)
		return 0;

	size_t colonPos = json.find(":", keyPos);
	if (colonPos == string::npos)
		return 0;

	size_t numStart = colonPos + 1;
	while (numStart < json.length() && (json[numStart] == ' ' || json[numStart] == '\t' || json[numStart] == '\n'))
		numStart++;

	size_t numEnd = numStart;
	while (numEnd < json.length() && (isdigit(json[numEnd]) || json[numEnd] == '-'))
		numEnd++;

	if (numEnd > numStart)
	{
		string numStr = json.substr(numStart, numEnd - numStart);
		return stoi(numStr);
	}

	return 0;
}

vector<int32> RoomDataManager::GetIntArray(const string& json, const string& key, size_t startPos)
{
	vector<int32> result;

	size_t keyPos = (key.empty()) ? startPos : FindKey(json, key, startPos);
	if (keyPos == string::npos)
		return result;

	size_t colonPos = (key.empty()) ? keyPos : json.find(":", keyPos);
	size_t arrayStart = json.find("[", colonPos);
	if (arrayStart == string::npos)
		return result;

	size_t arrayEnd = json.find("]", arrayStart);
	if (arrayEnd == string::npos)
		return result;

	size_t pos = arrayStart + 1;
	while (pos < arrayEnd)
	{
		while (pos < arrayEnd && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == ','))
			pos++;

		if (pos >= arrayEnd)
			break;

		size_t numEnd = pos;
		while (numEnd < arrayEnd && (isdigit(json[numEnd]) || json[numEnd] == '-'))
			numEnd++;

		if (numEnd > pos)
		{
			string numStr = json.substr(pos, numEnd - pos);
			result.push_back(stoi(numStr));
			pos = numEnd;
		}
		else
		{
			pos++;
		}
	}

	return result;
}

string RoomDataManager::GetObjectBlock(const string& json, size_t startPos, size_t& outEndPos)
{
	size_t objStart = json.find("{", startPos);
	if (objStart == string::npos)
		return "";

	int braceCount = 1;
	size_t pos = objStart + 1;

	while (pos < json.length() && braceCount > 0)
	{
		if (json[pos] == '{')
			braceCount++;
		else if (json[pos] == '}')
			braceCount--;

		pos++;
	}

	if (braceCount == 0)
	{
		outEndPos = pos;
		return json.substr(objStart, pos - objStart);
	}

	return "";
}

vector<string> RoomDataManager::GetObjectsInArray(const string& json, const string& arrayKey, size_t startPos)
{
	vector<string> result;

	size_t keyPos = FindKey(json, arrayKey, startPos);
	if (keyPos == string::npos)
	{
		cout << "[RoomDataManager] GetObjectsInArray: Key not found: " << arrayKey << endl;
		return result;
	}

	size_t colonPos = json.find(":", keyPos);
	if (colonPos == string::npos)
		return result;

	size_t arrayStart = json.find("[", colonPos);
	if (arrayStart == string::npos)
		return result;

	// 배열의 끝을 찾기 위해 브래킷 카운팅
	int bracketCount = 1;
	size_t arrayEnd = arrayStart + 1;
	while (arrayEnd < json.length() && bracketCount > 0)
	{
		if (json[arrayEnd] == '[')
			bracketCount++;
		else if (json[arrayEnd] == ']')
			bracketCount--;
		arrayEnd++;
	}

	cout << "[RoomDataManager] GetObjectsInArray: Array range [" << arrayStart << ", " << arrayEnd << "]" << endl;

	size_t pos = arrayStart + 1;
	int objCount = 0;
	while (pos < arrayEnd)
	{
		// 공백 건너뛰기
		while (pos < arrayEnd && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == ','))
			pos++;

		if (pos >= arrayEnd || json[pos] == ']')
			break;

		size_t endPos = 0;
		string objBlock = GetObjectBlock(json, pos, endPos);

		if (objBlock.empty())
		{
			cout << "[RoomDataManager] GetObjectsInArray: Empty object block at pos " << pos << endl;
			break;
		}

		objCount++;
		cout << "[RoomDataManager] GetObjectsInArray: Found object #" << objCount << " (size=" << objBlock.size() << ")" << endl;

		result.push_back(objBlock);
		pos = endPos;
	}

	cout << "[RoomDataManager] GetObjectsInArray: Total objects found: " << result.size() << endl;

	return result;
}

bool RoomDataManager::ParseBool(const string& value)
{
	return value == "true" || value == "TRUE" || value == "1";
}

const RoomConfigData* RoomDataManager::GetRoomConfig(const string& roomId) const
{
	auto it = _roomConfigs.find(roomId);
	if (it == _roomConfigs.end())
		return nullptr;
	return &it->second;
}

const RoomSpawnConfig* RoomDataManager::GetSpawnConfig(const string& roomId) const
{
	auto it = _spawnConfigs.find(roomId);
	if (it == _spawnConfigs.end())
		return nullptr;
	return &it->second;
}

const MonsterTemplateData* RoomDataManager::GetMonsterTemplate(int32 templateId) const
{
	auto it = _monsterTemplates.find(templateId);
	if (it == _monsterTemplates.end())
		return nullptr;
	return &it->second;
}

bool RoomDataManager::LoadLevelData(const string& csvPath)
{
	ifstream file(csvPath);
	if (!file.is_open())
	{
		cout << "[RoomDataManager] ERROR: Failed to open " << csvPath << endl;
		return false;
	}

	string line;
	getline(file, line); // 헤더 스킵

	while (getline(file, line))
	{
		if (line.empty()) continue;

		stringstream ss(line);
		string token;
		LevelData data;

		// level
		getline(ss, token, ',');
		data.level = stoi(token);

		// requiredExp
		getline(ss, token, ',');
		data.requiredExp = stoi(token);

		// maxHp
		getline(ss, token, ',');
		data.maxHp = stoi(token);

		// attack
		getline(ss, token, ',');
		data.attack = stoi(token);

		// defence
		getline(ss, token, ',');
		data.defence = stoi(token);

		_levelData[data.level] = data;
	}

	cout << "[RoomDataManager] Loaded " << _levelData.size() << " level data entries" << endl;
	return true;
}

const LevelData* RoomDataManager::GetLevelData(int32 level) const
{
	auto it = _levelData.find(level);
	if (it == _levelData.end())
		return nullptr;
	return &it->second;
}

vector<string> RoomDataManager::GetStaticRoomIds() const
{
	vector<string> result;
	for (const auto& pair : _roomConfigs)
	{
		if (!pair.second.isInstance)
			result.push_back(pair.first);
	}
	return result;
}

vector<string> RoomDataManager::GetInstanceRoomIds() const
{
	vector<string> result;
	for (const auto& pair : _roomConfigs)
	{
		if (pair.second.isInstance)
			result.push_back(pair.first);
	}
	return result;
}

wstring RoomDataManager::StringToWString(const string& str)
{
	if (str.empty())
		return wstring();

	// UTF-8 string to wstring 변환 (Win32 API)
	int wideSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (wideSize <= 0)
		return wstring();

	wstring result(wideSize - 1, L'\0'); // null terminator 제외
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], wideSize);

	return result;
}

bool RoomDataManager::LoadItemTemplates(const string& csvPath)
{
	ifstream file(csvPath);
	if (!file.is_open())
	{
		cout << "[RoomDataManager] ERROR: Failed to open " << csvPath << endl;
		return false;
	}

	string line;
	getline(file, line); // 헤더 스킵

	while (getline(file, line))
	{
		if (line.empty()) continue;

		stringstream ss(line);
		string token;
		ItemTemplateData data;

		getline(ss, token, ',');
		data.itemId = stoi(token);

		getline(ss, data.name, ',');

		getline(ss, data.type, ',');

		getline(ss, token, ',');
		data.value = stoi(token);

		getline(ss, token, ',');
		data.maxStack = stoi(token);

		_itemTemplates[data.itemId] = data;
	}

	cout << "[RoomDataManager] Loaded " << _itemTemplates.size() << " item templates" << endl;
	return true;
}

bool RoomDataManager::LoadMonsterDrops(const string& csvPath)
{
	ifstream file(csvPath);
	if (!file.is_open())
	{
		cout << "[RoomDataManager] ERROR: Failed to open " << csvPath << endl;
		return false;
	}

	string line;
	getline(file, line); // 헤더 스킵

	while (getline(file, line))
	{
		if (line.empty()) continue;

		stringstream ss(line);
		string token;
		MonsterDropData data;

		getline(ss, token, ',');
		data.templateId = stoi(token);

		getline(ss, token, ',');
		data.itemId = stoi(token);

		getline(ss, token, ',');
		data.dropRate = stoi(token);

		_monsterDrops[data.templateId].push_back(data);
	}

	cout << "[RoomDataManager] Loaded monster drops for " << _monsterDrops.size() << " monster types" << endl;
	return true;
}

const ItemTemplateData* RoomDataManager::GetItemTemplate(int32 itemId) const
{
	auto it = _itemTemplates.find(itemId);
	if (it == _itemTemplates.end())
		return nullptr;
	return &it->second;
}

const vector<MonsterDropData>* RoomDataManager::GetMonsterDrops(int32 templateId) const
{
	auto it = _monsterDrops.find(templateId);
	if (it == _monsterDrops.end())
		return nullptr;
	return &it->second;
}
