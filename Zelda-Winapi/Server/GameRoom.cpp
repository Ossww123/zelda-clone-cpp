#include "pch.h"
#include "GameRoom.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "GameSession.h"
#include "GameRoomManager.h"
#include "RoomConfig.h"
#include "RoomDataManager.h"
#include "PartyManager.h"

GameRoom::GameRoom() : _combat(*this), _spawn(*this)
{
}

GameRoom::~GameRoom()
{
}

void GameRoom::InitFromConfig(const string& roomId)
{
	_roomIdStr = roomId;
	_config = GRoomDataManager.GetRoomConfig(roomId);
	const RoomSpawnConfig* spawnConfig = GRoomDataManager.GetSpawnConfig(roomId);

	if (!_config)
	{
		LOG_ERROR("Room", "Room config not found for: %s", roomId.c_str());
		return;
	}

	LoadMap(_config->tilemapPath.c_str());
	_spawn.Init(_config, spawnConfig);

	LOG_INFO("Room", "Initialized room: %s (SkillEnabled=%d, MonsterSpawn=%d)",
		roomId.c_str(), _config->skillEnabled, _config->monsterSpawnEnabled);
}

void GameRoom::Init()
{
}

void GameRoom::Update(uint64 now)
{
	{
		queue<function<void()>> jobs;
		{
			LockGuard guard(_jobLock);
			std::swap(jobs, _jobs);
		}

		while (!jobs.empty())
		{
			jobs.front()();
			jobs.pop();
		}
	}

	if (_nextTick == 0)
		_nextTick = now + kTickMs;

	uint32 steps = 0;
	while (now >= _nextTick && steps < kMaxCatchUp)
	{
		Step(_nextTick);
		_nextTick += kTickMs;
		steps++;
	}

	// 너무 많이 밀리면 리턴
	if (now >= _nextTick)
		_nextTick = now + kTickMs;
}

void GameRoom::Step(uint64 now)
{
	for (auto& item : _players)
	{
		item.second->Update();
	}

	for (auto& item : _monsters)
	{
		item.second->Update();
	}

	for (auto it = _projectiles.begin(); it != _projectiles.end(); )
	{
		GameObjectRef obj = it->second;
		++it;
		obj->Update();
	}

	_spawn.Tick();
}

void GameRoom::EnterRoom(GameSessionRef session)
{
	PlayerRef player = GameObject::CreatePlayer();
	EnterRoom(session, player);
}

void GameRoom::EnterRoom(GameSessionRef session, PlayerRef player)
{
	session->gameRoom = GetRoomRef();
	session->player = player;
	player->session = session;

	player->info.set_posx(5);
	player->info.set_posy(5);
	player->info.set_state(IDLE);
	player->StopMove();

	{
		Protocol::S_MyPlayer myPlayerPkt;
		*myPlayerPkt.mutable_info() = player->info;
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_MyPlayer(myPlayerPkt);
		session->Send(sendBuffer);
	}

	player->SendInventoryData();

	{
		Protocol::S_AddObject pkt;

		for (auto item : _players)
		{
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		for (auto item : _monsters)
		{
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		for (auto item : _projectiles)
		{
			Protocol::ObjectInfo* info = pkt.add_objects();
			*info = item.second->info;
		}

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
		session->Send(sendBuffer);
	}

	AddObject(player);

	// 파티 소속이면 같은 방의 파티원에게 파티 정보 갱신
	uint64 playerId = player->info.objectid();
	uint64 partyId = GPartyManager.GetPartyIdByPlayer(playerId);
	if (partyId != 0)
	{
		Party* party = GPartyManager.GetParty(partyId);
		if (party)
		{
			Protocol::S_PartyUpdate partyPkt;
			for (uint64 memberId : party->memberIds)
			{
				Protocol::PartyMemberInfo* m = partyPkt.add_members();
				m->set_playerid(memberId);
				m->set_isleader(memberId == party->leaderId);

				GameObjectRef obj = FindObject(memberId);
				if (obj)
				{
					PlayerRef p = static_pointer_cast<Player>(obj);
					m->set_name(p->info.name());
					m->set_level(p->GetLevel());
					m->set_hp(p->info.hp());
					m->set_maxhp(p->info.maxhp());
				}
				else
				{
					auto nameIt = party->memberNames.find(memberId);
					string name = (nameIt != party->memberNames.end()) ? nameIt->second : "Unknown";
					m->set_name(name + "(Away)");
				}
			}
			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(partyPkt, S_PartyUpdate);

			for (uint64 memberId : party->memberIds)
			{
				auto it = _players.find(memberId);
				if (it != _players.end() && it->second->session)
					it->second->session->Send(sendBuffer);
			}
		}
	}
}

void GameRoom::LeaveRoom(GameSessionRef session)
{
	if (session == nullptr)
		return;
	if (session->player.lock() == nullptr)
		return;

	uint64 id = session->player.lock()->info.objectid();
	RemoveObject(id);

	if (IsDungeonInstance() && GetPlayerCount() == 0)
	{
		uint64 instanceId = GetInstanceId();
		if (instanceId != 0)
		{
			GRoomManager.RequestRemoveDungeonInstance(instanceId);
			LOG_INFO("Room", "Requested removal of empty dungeon instance: %llu", instanceId);
		}
	}
}

GameObjectRef GameRoom::FindObject(uint64 id)
{
	{
		auto findIt = _players.find(id);
		if (findIt != _players.end())
			return findIt->second;
	}
	{
		auto findIt = _monsters.find(id);
		if (findIt != _monsters.end())
			return findIt->second;
	}
	{
		auto findIt = _projectiles.find(id);
		if (findIt != _projectiles.end())
			return findIt->second;
	}

	return nullptr;
}

void GameRoom::LoadMap(const wchar_t* path)
{
	_tilemap.LoadFile(path);
}

void GameRoom::Handle_C_Move(GameSessionRef session, const Protocol::C_Move& pkt)
{
	PlayerRef player = session->player.lock();
	if (!player)
		return;

	if (player->info.state() == MOVE)
		return;

	const auto dir = pkt.dir();
	if (!Protocol::DIR_TYPE_IsValid(static_cast<int>(dir)))
		return;

	if (player->info.dir() != dir)
	{
		player->info.set_dir(dir);
		player->info.set_state(IDLE);

		Protocol::S_Turn turnPkt;
		*turnPkt.mutable_info() = player->info;

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Turn(turnPkt);
		Broadcast(sendBuffer);
		return;
	}

	Vec2Int nextPos = player->GetFrontCellPos();
	if (!CanGo(nextPos))
	{
		player->info.set_state(IDLE);
		Protocol::S_Move movePkt;
		*movePkt.mutable_info() = player->info;
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(movePkt);
		Broadcast(sendBuffer);
		return;
	}

	player->info.set_state(MOVE);
	player->info.set_posx(nextPos.x);
	player->info.set_posy(nextPos.y);

	// TEMP: 고정 이동 시간
	constexpr uint64 kMoveDurationMs = 75;
	uint64 now = GetTickCount64();
	player->StartMove(now, kMoveDurationMs);

	Protocol::S_Move movePkt;
	*movePkt.mutable_info() = player->info;
	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(movePkt);
	Broadcast(sendBuffer);
}


void GameRoom::Handle_C_Turn(GameSessionRef session, const Protocol::C_Turn& pkt)
{
	PlayerRef player = session->player.lock();
	if (!player)
		return;

	const auto dir = pkt.dir();
	if (!Protocol::DIR_TYPE_IsValid(static_cast<int>(dir)))
		return;

	// 이미 같은 방향
	if (player->info.dir() == dir)
		return;

	player->info.set_dir(dir);
	player->info.set_state(IDLE);

	Protocol::S_Turn turnPkt;
	*turnPkt.mutable_info() = player->info;

	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Turn(turnPkt);
	Broadcast(sendBuffer);
}


void GameRoom::Handle_C_Attack(GameSessionRef session, const Protocol::C_Attack& pkt)
{
	PlayerRef attacker = session->player.lock();
	if (!attacker)
		return;

	_combat.HandleAttack(attacker, pkt);
}

void GameRoom::Handle_C_ChangeMap(GameSessionRef session, const Protocol::C_ChangeMap& pkt)
{
	PlayerRef requester = dynamic_pointer_cast<Player>(session->player.lock());
	if (!requester)
		return;

	uint64 requesterId = requester->info.objectid();
	GameRoomRef from = shared_from_this();

	GameRoomRef to = nullptr;
	uint64 instanceId = 0;

	if (pkt.mapid() == Protocol::MAP_ID_TOWN)
	{
		to = GRoomManager.GetStaticRoom(FieldId::Town, pkt.channel());
	}
	else if (pkt.mapid() == Protocol::MAP_ID_DUNGEON)
	{
		// 파티원(비리더)이면 던전 진입 불가
		if (GPartyManager.IsInParty(requesterId) && !GPartyManager.IsLeader(requesterId))
		{
			Protocol::S_ChangeMap sendPkt;
			sendPkt.set_success(false);
			sendPkt.set_mapid(pkt.mapid());
			sendPkt.set_channel(pkt.channel());
			sendPkt.set_instanceid(0);
			session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
			return;
		}

		instanceId = GRoomManager.CreateDungeonInstance();
		to = GRoomManager.GetDungeonInstance(instanceId);
	}

	if (!to)
	{
		Protocol::S_ChangeMap sendPkt;
		sendPkt.set_success(false);
		sendPkt.set_mapid(pkt.mapid());
		sendPkt.set_channel(pkt.channel());
		sendPkt.set_instanceid(0);
		session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
		return;
	}

	// 파티장이 던전 진입 시 → 같은 방의 파티원 전원 이동
	if (pkt.mapid() == Protocol::MAP_ID_DUNGEON && GPartyManager.IsLeader(requesterId))
	{
		uint64 partyId = GPartyManager.GetPartyIdByPlayer(requesterId);
		Party* party = GPartyManager.GetParty(partyId);
		if (party)
		{
			vector<pair<GameSessionRef, PlayerRef>> toMove;
			for (uint64 memberId : party->memberIds)
			{
				GameObjectRef obj = from->FindObject(memberId);
				if (!obj)
					continue;
				PlayerRef p = dynamic_pointer_cast<Player>(obj);
				if (p && p->session)
					toMove.push_back({ p->session, p });
			}

			for (auto& [s, p] : toMove)
			{
				Protocol::S_ChangeMap sendPkt;
				sendPkt.set_success(true);
				sendPkt.set_mapid(pkt.mapid());
				sendPkt.set_channel(pkt.channel());
				sendPkt.set_instanceid(instanceId);
				s->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));

				from->LeaveRoom(s);
				to->PushJob([to, s, p]()
					{
						to->EnterRoom(s, p);
					});
			}
			return;
		}
	}

	// 솔로 이동 — 기존 플레이어 유지
	{
		Protocol::S_ChangeMap sendPkt;
		sendPkt.set_success(true);
		sendPkt.set_mapid(pkt.mapid());
		sendPkt.set_channel(pkt.channel());
		sendPkt.set_instanceid(instanceId);
		session->Send(ServerPacketHandler::Make_S_ChangeMap(sendPkt));
	}

	// LeaveRoom 전에 플레이어 강한 참조 확보
	PlayerRef movingPlayer = dynamic_pointer_cast<Player>(session->player.lock());
	from->LeaveRoom(session);

	to->PushJob([to, session, movingPlayer]()
		{
			to->EnterRoom(session, movingPlayer);
		});
}

void GameRoom::PushJob(function<void()> job)
{
	LockGuard guard(_jobLock);
	_jobs.push(std::move(job));
}

void GameRoom::AddObject(GameObjectRef gameObject)
{
	uint64 id = gameObject->info.objectid();

	auto objectType = gameObject->info.objecttype();

	switch (objectType)
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_players[id] = static_pointer_cast<Player>(gameObject);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_monsters[id] = static_pointer_cast<Monster>(gameObject);
		break;
	case Protocol::OBJECT_TYPE_PROJECTILE:
		_projectiles[id] = static_pointer_cast<Projectile>(gameObject);
		break;
	default:
		return;
	}

	gameObject->room = GetRoomRef();

	{
		Protocol::S_AddObject pkt;

		Protocol::ObjectInfo* info = pkt.add_objects();
		*info = gameObject->info;
		
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
		Broadcast(sendBuffer);
	}
}

void GameRoom::RemoveObject(uint64 id)
{
	GameObjectRef gameObject = FindObject(id);
	if (gameObject == nullptr)
		return;

	if (gameObject->info.objecttype() == Protocol::OBJECT_TYPE_MONSTER)
	{
		MonsterRef monster = static_pointer_cast<Monster>(gameObject);
		_spawn.TryReserve(monster);
	}

	switch (gameObject->info.objecttype())
	{
	case Protocol::OBJECT_TYPE_PLAYER:
		_players.erase(id);
		break;
	case Protocol::OBJECT_TYPE_MONSTER:
		_monsters.erase(id);
		break;
	case Protocol::OBJECT_TYPE_PROJECTILE:
		_projectiles.erase(id);
		break;
	default:
		return;
	}

	gameObject->room = nullptr;

	{
		Protocol::S_RemoveObject pkt;
		pkt.add_ids(id);

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
		Broadcast(sendBuffer);
	}
}

void GameRoom::Broadcast(SendBufferRef& sendBuffer)
{
	for (auto& item : _players)
	{
		item.second->session->Send(sendBuffer);
	}
}

PlayerRef GameRoom::FindClosestPlayer(Vec2Int pos)
{
	float best = FLT_MAX;
	PlayerRef ret = nullptr;

	for (auto& item : _players)
	{
		PlayerRef player = item.second;
		if (player)
		{
			Vec2Int dir = pos - player->GetCellPos();
			float dist = dir.LengthSquared();
			if (dist < best)
			{
				best = dist;
				ret = player;
			}
		}
	}

	return ret;
}

bool GameRoom::FindPath(Vec2Int src, Vec2Int dest, vector<Vec2Int>& path, int32 maxDepth /*= 10*/)
{
	int32 depth = abs(src.y - dest.y) + abs(src.x - dest.x);
	if (depth >= maxDepth)
		return false;

	priority_queue<PQNode, vector<PQNode>, greater<PQNode>> pq;
	map<Vec2Int, int32> best;
	map<Vec2Int, Vec2Int> parent;

	{
		int32 cost = abs(dest.y - src.y) + abs(dest.x - src.x);

		pq.push(PQNode(cost, src));
		best[src] = cost;
		parent[src] = src;
	}

	Vec2Int front[4] =
	{
		{0, -1},
		{0, 1},
		{-1, 0},
		{1, 0},
	};

	bool found = false;

	while (pq.empty() == false)
	{
		PQNode node = pq.top();
		pq.pop();

		if (best[node.pos] < node.cost)
			continue;

		if (node.pos == dest)
		{
			found = true;
			break;
		}

		for (int32 dir = 0; dir < 4; dir++)
		{
			Vec2Int nextPos = node.pos + front[dir];

			if (CanGo(nextPos) == false)
				continue;

			int32 depth = abs(src.y - nextPos.y) + abs(src.x - nextPos.x);
			if (depth >= maxDepth)
				continue;

			int32 cost = abs(dest.y - nextPos.y) + abs(dest.x - nextPos.x);
			int32 bestValue = best[nextPos];
			if (bestValue != 0)
			{
				if (bestValue <= cost)
					continue;
			}

			best[nextPos] = cost;
			pq.push(PQNode(cost, nextPos));
			parent[nextPos] = node.pos;
		}
	}

	if (found == false)
	{
		float bestScore = FLT_MAX;

		for (auto& item : best)
		{
			Vec2Int pos = item.first;
			int32 score = item.second;

			if (bestScore == score)
			{
				int32 dist1 = abs(dest.x - src.x) + abs(dest.y - src.y);
				int32 dist2 = abs(pos.x - src.x) + abs(pos.y - src.y);
				if (dist1 > dist2)
					dest = pos;
			}
			else if (bestScore > score)
			{
				dest = pos;
				bestScore = score;
			}
		}
	}

	path.clear();
	Vec2Int pos = dest;

	while (true)
	{
		path.push_back(pos);

		if (pos == parent[pos])
			break;

		pos = parent[pos];
	}

	std::reverse(path.begin(), path.end());
	return true;
}

bool GameRoom::CanGo(Vec2Int cellPos)
{
	Tile* tile = _tilemap.GetTileAt(cellPos);
	if (tile == nullptr)
		return false;

	if (GetCreatureAt(cellPos) != nullptr)
		return false;

	return tile->value != 1;
}

Vec2Int GameRoom::GetRandomEmptyCellPos()
{
	Vec2Int ret = { -1, -1 };

	Vec2Int size = _tilemap.GetMapSize();

	while (true)
	{
		int32 x = rand() % size.x;
		int32 y = rand() % size.y;
		Vec2Int cellPos{ x, y };

		if (CanGo(cellPos))
			return cellPos;
	}
}

bool GameRoom::IsBlockedByWall(Vec2Int cellPos)
{
	Tile* tile = _tilemap.GetTileAt(cellPos);
	if (tile == nullptr)
		return true;

	return tile->value == 1;
}

GameObjectRef GameRoom::GetGameObjectAt(Vec2Int cellPos)
{
	for (auto& item : _players)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	for (auto& item : _monsters)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	for (auto& item : _projectiles)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	return nullptr;
}

CreatureRef GameRoom::GetCreatureAt(Vec2Int cellPos)
{
	for (auto& item : _players)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	for (auto& item : _monsters)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	return nullptr;
}

MonsterRef GameRoom::GetMonsterAt(Vec2Int cellPos)
{
	for (auto& item : _monsters)
	{
		if (item.second->GetCellPos() == cellPos)
			return item.second;
	}

	return nullptr;
}

void GameRoom::DistributeExp(PlayerRef killer, MonsterRef monster)
{
	_combat.DistributeExp(killer, monster);
}

void GameRoom::ProcessMonsterDrop(PlayerRef killer, MonsterRef monster)
{
	_combat.ProcessMonsterDrop(killer, monster);
}

bool GameRoom::CanUseSkill() const
{
	return _config && _config->skillEnabled;
}

