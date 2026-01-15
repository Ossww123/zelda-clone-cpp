#pragma once
#include "Tilemap.h"

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

	void Init();
	void Update();

	void EnterRoom(GameSessionRef session);
	void LeaveRoom(GameSessionRef session);
	GameObjectRef FindObject(uint64 id);
	GameRoomRef GetRoomRef() { return shared_from_this(); }

public:
	// PacketHandler
	void Handle_C_Move(GameSessionRef session, const Protocol::C_Move& pkt);
	void Handle_C_Attack(GameSessionRef session, const Protocol::C_Attack& pkt);

public:
	void PushJob(function<void()> job);

public:
	void AddObject(GameObjectRef gameObject);
	void RemoveObject(uint64 id);
	void Broadcast(SendBufferRef& sendBuffer);

public:
	PlayerRef FindClosestPlayer(Vec2Int pos);
	bool FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth = 10);
	bool CanGo(Vec2Int cellPos);
	Vec2Int GetRandomEmptyCellPos();
	bool IsBlockedByWall(Vec2Int cellPos);
	GameObjectRef GetGameObjectAt(Vec2Int cellPos);
	CreatureRef GetCreatureAt(Vec2Int cellPos);
	MonsterRef GetMonsterAt(Vec2Int cellPos);
	
private:
	void Handle_SwordAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void Handle_BowAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void BroadcastAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void BroadcastDamaged(PlayerRef attacker, CreatureRef target, int32 damage);

private:
	map<uint64, PlayerRef> _players;
	map<uint64, MonsterRef> _monsters;
	map<uint64, GameObjectRef> _projectiles;
	Tilemap _tilemap;

public:
	void SetRoomId(uint64 id) { _roomId = id; }
	uint64 GetRoomId() const { return _roomId; }

private:
	uint64 _roomId = 0;

	Mutex _jobLock;
	queue<function<void()>> _jobs;
};