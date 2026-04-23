#pragma once

struct RoomConfigData;
struct RoomSpawnConfig;
class GameRoom;

class SpawnSystem
{
public:
	SpawnSystem(GameRoom& room);

	void Init(const RoomConfigData* config, const RoomSpawnConfig* spawnConfig);
	void Tick();
	void TryReserve(MonsterRef monster);

	bool ShouldSpawn() const;

private:
	void SpawnMonstersFromData();
	void ProcessRespawn();

	struct RespawnRequest
	{
		uint64 when;
		Vec2Int homePos;
		int32 templateId;
		int32 level;
		int32 aggroRange;
		int32 leashRange;
	};

	GameRoom& _room;
	const RoomConfigData* _config = nullptr;
	const RoomSpawnConfig* _spawnConfig = nullptr;
	vector<RespawnRequest> _respawnQueue;
};
