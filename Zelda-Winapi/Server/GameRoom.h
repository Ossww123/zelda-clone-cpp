#pragma once
#include "Tilemap.h"
#include "CombatSystem.h"
#include "SpawnSystem.h"

struct RoomConfigData;
struct RoomSpawnConfig;

struct PQNode
{
	PQNode(int32 cost, Vec2Int pos) : cost(cost), pos(pos) { }

	bool operator<(const PQNode& other) const { return cost < other.cost; }
	bool operator>(const PQNode& other) const { return cost > other.cost; }

	int32 cost;
	Vec2Int pos;
};

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	virtual ~GameRoom();

	void InitFromConfig(const string& roomId);

	void Init();
	void Update(uint64 now);

private:
	void Step(uint64 now);

public:
	void EnterRoom(GameSessionRef session);
	void EnterRoom(GameSessionRef session, PlayerRef existingPlayer);
	void LeaveRoom(GameSessionRef session);
	GameObjectRef FindObject(uint64 id);
	GameRoomRef GetRoomRef() { return shared_from_this(); }
	void LoadMap(const wchar_t* path);

public:
	void SetFieldId(FieldId id) { _fieldId = id; }
	FieldId GetFieldId() const { return _fieldId; }

	void SetChannel(int32 ch) { _channel = ch; }
	int32 GetChannel() const { return _channel; }

	void SetInstanceId(uint64 id) { _instanceId = id; }
	uint64 GetInstanceId() const { return _instanceId; }

	bool IsDungeonInstance() const { return _fieldId == FieldId::Dungeon; }
	int32 GetPlayerCount() const { return static_cast<int32>(_players.size()); }

	bool CanUseSkill() const;

	const map<uint64, PlayerRef>& GetPlayers() const { return _players; }

public:
	// PacketHandler
	void Handle_C_Move(GameSessionRef session, const Protocol::C_Move& pkt);
	void Handle_C_Turn(GameSessionRef session, const Protocol::C_Turn& pkt);
	void Handle_C_Attack(GameSessionRef session, const Protocol::C_Attack& pkt);
	void Handle_C_ChangeMap(GameSessionRef session, const Protocol::C_ChangeMap& pkt);

public:
	void PushJob(function<void()> job);

public:
	void AddObject(GameObjectRef gameObject);
	void RemoveObject(uint64 id);
	void Broadcast(SendBufferRef& sendBuffer);
	void DistributeExp(PlayerRef killer, MonsterRef monster);
	void ProcessMonsterDrop(PlayerRef killer, MonsterRef monster);

public:
	PlayerRef FindClosestPlayer(Vec2Int pos);
	bool FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool CanGo(Vec2Int cellPos);
	Vec2Int GetRandomEmptyCellPos();
	bool IsBlockedByWall(Vec2Int cellPos);
	GameObjectRef GetGameObjectAt(Vec2Int cellPos);
	CreatureRef GetCreatureAt(Vec2Int cellPos);
	MonsterRef GetMonsterAt(Vec2Int cellPos);

public:
	void SetRoomId(uint64 id) { _roomId = id; }
	uint64 GetRoomId() const { return _roomId; }

private:
	map<uint64, PlayerRef> _players;
	map<uint64, MonsterRef> _monsters;
	map<uint64, GameObjectRef> _projectiles;
	Tilemap _tilemap;

	uint64 _roomId = 0;
	FieldId _fieldId = FieldId::Town;
	int32 _channel = 0;
	uint64 _instanceId = 0;

	Mutex _jobLock;
	queue<function<void()>> _jobs;

	const RoomConfigData* _config = nullptr;
	string _roomIdStr;

	CombatSystem _combat;
	SpawnSystem _spawn;

	static constexpr uint32 kTickMs = 50;
	static constexpr uint32 kMaxCatchUp = 5;
	uint64 _nextTick = 0;
};
