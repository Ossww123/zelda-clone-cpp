#include "pch.h"
#include "GameRoom.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "Arrow.h"
#include "GameSession.h"
#include "GameRoomManager.h"
#include "RoomConfig.h"
#include "RoomDataManager.h"

GameRoom::GameRoom()
{
}

GameRoom::~GameRoom()
{
}

void GameRoom::InitFromConfig(const string& roomId)
{
	_roomIdStr = roomId;
	_config = GRoomDataManager.GetRoomConfig(roomId);
	_spawnConfig = GRoomDataManager.GetSpawnConfig(roomId);

	if (!_config)
	{
		cout << "[GameRoom] ERROR: Room config not found for: " << roomId << endl;
		return;
	}

	// 타일맵 로드
	LoadMap(_config->tilemapPath.c_str());

	// 몬스터 스폰 (설정에 따라)
	if (_config->monsterSpawnEnabled && _spawnConfig)
	{
		SpawnMonstersFromData();
	}

	cout << "[GameRoom] Initialized room: " << roomId << " (SkillEnabled=" << _config->skillEnabled
		<< ", MonsterSpawn=" << _config->monsterSpawnEnabled << ")" << endl;
}

void GameRoom::Init()
{
	// 레거시 로직 제거됨
}

void GameRoom::Update()
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

	// 데이터 기반 리스폰 처리
	if (ShouldSpawnMonsters())
	{
		ProcessRespawnFromData();
	}
}

void GameRoom::EnterRoom(GameSessionRef session)
{
	PlayerRef player = GameObject::CreatePlayer();

	// ������ ���縦 ����/
	session->gameRoom = GetRoomRef();
	session->player = player;
	player->session = session;

	// TEMP
	player->info.set_posx(5);
	player->info.set_posy(5);

	// ������ Ŭ�󿡰� ������ �����ֱ�
	{
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_MyPlayer(player->info);
		session->Send(sendBuffer);
	}
	// ��� ������Ʈ ���� ����
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
}

void GameRoom::LeaveRoom(GameSessionRef session)
{
	if (session == nullptr)
		return;
	if (session->player.lock() == nullptr)
		return;

	uint64 id = session->player.lock()->info.objectid();
	RemoveObject(id);

	// 빈 던전 인스턴스 자동 제거
	if (IsDungeonInstance() && GetPlayerCount() == 0)
	{
		uint64 instanceId = GetInstanceId();
		if (instanceId != 0)
		{
			GRoomManager.RequestRemoveDungeonInstance(instanceId);
			cout << "[GameRoom] Requested removal of empty dungeon instance: " << instanceId << endl;
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

	GameObjectRef gameObject = player;
	if (gameObject == nullptr)
		return;

	// TODO : Validation

	gameObject->info.set_state(MOVE);
	gameObject->info.set_dir(pkt.dir());
	gameObject->info.set_posx(pkt.targetx());
	gameObject->info.set_posy(pkt.targety());

	{
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(gameObject->info);
		Broadcast(sendBuffer);
	}
}

void GameRoom::Handle_C_Attack(GameSessionRef session, const Protocol::C_Attack& pkt)
{
	PlayerRef attacker = session->player.lock();
	if (!attacker)
		return;

	// 스킬 사용 가능 여부 체크 (타운에서는 금지)
	if (!CanUseSkill())
	{
		cout << "[GameRoom] Attack blocked in room: " << _roomIdStr << " (SkillEnabled=false)" << endl;
		return;
	}

	BroadcastAttack(attacker, pkt);

	Protocol::WEAPON_TYPE weaponType = pkt.weapontype();

	switch (pkt.weapontype())
	{
	case Protocol::WEAPON_TYPE_SWORD:
		Handle_SwordAttack(attacker, pkt);
		break;

	case Protocol::WEAPON_TYPE_BOW:
		Handle_BowAttack(attacker, pkt);
		break;

	case Protocol::WEAPON_TYPE_STAFF:
		Handle_StaffAttack(attacker, pkt);
		break;

	default:
		break;
	}
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

	// �ű� ������Ʈ ���� ����
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

	// 데이터 기반 몬스터 리스폰 예약
	if (gameObject->info.objecttype() == Protocol::OBJECT_TYPE_MONSTER && ShouldSpawnMonsters())
	{
		MonsterRef monster = static_pointer_cast<Monster>(gameObject);

		RespawnRequest req;
		req.when = GetTickCount64() + GetRespawnTime();
		req.homePos = monster->GetHomePos();
		req.templateId = monster->GetTemplateId();
		req.level = 1; // TODO: 레벨 시스템 구현 시 변경
		req.aggroRange = monster->GetAggroRange();
		req.leashRange = monster->GetLeashRange();

		ReserveMonsterRespawn(req);
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

	// ������Ʈ ���� ����
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
				dist = best;
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

	// �ʱⰪ
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
		// ���� ���� �ĺ��� ã�´�
		PQNode node = pq.top();
		pq.pop();

		// �� ª�� ��θ� �ڴʰ� ã�Ҵٸ� ��ŵ
		if (best[node.pos] < node.cost)
			continue;

		// �������� ���������� �ٷ� ����
		if (node.pos == dest)
		{
			found = true;
			break;
		}

		// �湮
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
				// �ٸ� ��ο��� �� ���� ���� ã������ ��ŵ
				if (bestValue <= cost)
					continue;
			}

			// ���� ����
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

			// �����̶��, ���� ��ġ���� ���� �� �̵��ϴ� ������
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

		// ������
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

	// �� �� �õ�?
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

void GameRoom::Handle_SwordAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	Vec2Int frontPos = attacker->GetFrontCellPos();
	GameObjectRef obj = GetGameObjectAt(frontPos);
	CreatureRef target = std::dynamic_pointer_cast<Creature>(obj);

	if (!target)
		return;

	int32 damage = 0;
	if (!target->OnDamaged(attacker, damage))
		return;

	BroadcastDamaged(attacker, target, damage);

	if (target->info.hp() == 0)
		RemoveObject(target->info.objectid());
}

void GameRoom::Handle_BowAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	ArrowRef arrow = GameObject::CreateArrow();
	arrow->info.set_dir(pkt.dir());

	Vec2Int start = attacker->GetCellPos();
	arrow->info.set_posx(start.x);
	arrow->info.set_posy(start.y);

	arrow->SetOwner(attacker->info.objectid());

	AddObject(arrow);
}

void GameRoom::Handle_StaffAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	if (!attacker)
		return;

	Vec2Int pos = attacker->GetCellPos();

	Vec2Int forward = { 0, 0 };
	switch (pkt.dir())
	{
	case Protocol::DIR_TYPE_UP:
		forward = { 0, -1 };
		break;
	case Protocol::DIR_TYPE_DOWN:
		forward = { 0,  1 };
		break;
	case Protocol::DIR_TYPE_LEFT:
		forward = { -1, 0 };
		break;
	case Protocol::DIR_TYPE_RIGHT:
		forward = { 1,  0 };
		break;
	default:
		break;
	}

	Vec2Int center = pos + forward;

	std::vector<uint64> deadTargets;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			Vec2Int cell = { center.x + dx, center.y + dy };

			MonsterRef target = GetMonsterAt(cell);
			if (!target)
				continue;

			int32 damage = 0;
			if (!target->OnDamaged(attacker, damage, 0.5f))
				continue;

			BroadcastDamaged(attacker, target, damage);

			if (target->info.hp() == 0)
				deadTargets.push_back(target->info.objectid());
		}
	}

	for (uint64 id : deadTargets)
	{
		RemoveObject(id);
	}
}


void GameRoom::BroadcastAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	Protocol::S_Attack atk;
	atk.set_attackerid(attacker->info.objectid());
	atk.set_dir(pkt.dir());
	atk.set_weapontype(pkt.weapontype());

	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Attack(atk);
	Broadcast(sendBuffer);
}

void GameRoom::BroadcastDamaged(PlayerRef attacker, CreatureRef target, int32 damage)
{
	Protocol::S_Damaged dmgPkt;
	dmgPkt.set_attackerid(attacker->info.objectid());
	dmgPkt.set_targetid(target->info.objectid());
	dmgPkt.set_damage(damage);
	dmgPkt.set_newhp(target->info.hp());

	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Damaged(dmgPkt);
	Broadcast(sendBuffer);
}

// 데이터 기반 룸 설정 조회
bool GameRoom::CanUseSkill() const
{
	if (_config)
		return _config->skillEnabled;
	return true; // 기본값: 허용
}

bool GameRoom::ShouldSpawnMonsters() const
{
	if (_config)
		return _config->monsterSpawnEnabled;
	return false; // 기본값: 스폰 안함
}

uint32 GameRoom::GetRespawnTime() const
{
	if (_config)
		return _config->respawnTimeMs;
	return 10000; // 기본값: 10초
}

// 데이터 기반 몬스터 스폰
void GameRoom::SpawnMonstersFromData()
{
	if (!_spawnConfig)
	{
		cout << "[GameRoom] ERROR: _spawnConfig is null! Cannot spawn monsters." << endl;
		return;
	}

	cout << "[GameRoom] Starting monster spawn. Spawn groups: " << _spawnConfig->spawns.size() << endl;

	for (const auto& spawnGroup : _spawnConfig->spawns)
	{
		cout << "[GameRoom] Processing spawn group: " << spawnGroup.groupId
			<< " at (" << spawnGroup.anchor.x << ", " << spawnGroup.anchor.y << ")" << endl;

		Vec2Int anchor = spawnGroup.anchor;
		int offsetIndex = 0;

		cout << "[GameRoom] Monster types in this group: " << spawnGroup.monsters.size() << endl;

		for (const auto& monsterInfo : spawnGroup.monsters)
		{
			cout << "[GameRoom] Monster templateId=" << monsterInfo.templateId
				<< ", count=" << monsterInfo.count << endl;

			// MonsterTemplate에서 스탯 로드
			const MonsterTemplateData* templateData = GRoomDataManager.GetMonsterTemplate(monsterInfo.templateId);
			if (!templateData)
			{
				cout << "[GameRoom] ERROR: MonsterTemplate not found: " << monsterInfo.templateId << endl;
				continue;
			}

			for (int i = 0; i < monsterInfo.count; ++i)
			{
				if (offsetIndex >= spawnGroup.offsets.size())
				{
					cout << "[GameRoom] WARNING: Not enough offsets for monster count in group: " << spawnGroup.groupId << endl;
					break;
				}

				Vec2Int pos = anchor + spawnGroup.offsets[offsetIndex];
				offsetIndex++;

				if (!CanGo(pos))
					pos = GetRandomEmptyCellPos();

				MonsterRef m = GameObject::CreateMonster();
				m->SetHomePos(pos);
				m->SetCellPos(pos);
				m->SetAggroRange(monsterInfo.aggroRange);
				m->SetLeashRange(monsterInfo.leashRange);
				m->SetTemplateId(monsterInfo.templateId);

				// MonsterTemplate 스탯 적용
				m->info.set_name(templateData->name);
				m->info.set_maxhp(templateData->maxHp);
				m->info.set_hp(templateData->maxHp);
				m->info.set_attack(templateData->attack);
				m->info.set_defence(templateData->defence);

				AddObject(m);
			}
		}
	}

	cout << "[GameRoom] Spawned " << _monsters.size() << " monsters from data" << endl;
}

// 데이터 기반 리스폰 처리
void GameRoom::ReserveMonsterRespawn(const RespawnRequest& req)
{
	_respawnQueue.push_back(req);
}

void GameRoom::ProcessRespawnFromData()
{
	uint64 now = GetTickCount64();

	for (auto it = _respawnQueue.begin(); it != _respawnQueue.end(); )
	{
		if (now < it->when)
		{
			++it;
			continue;
		}

		Vec2Int pos = it->homePos;

		if (!CanGo(pos))
			pos = GetRandomEmptyCellPos();

		// MonsterTemplate에서 스탯 로드
		const MonsterTemplateData* templateData = GRoomDataManager.GetMonsterTemplate(it->templateId);

		MonsterRef m = GameObject::CreateMonster();
		m->SetHomePos(pos);
		m->SetCellPos(pos);
		m->SetAggroRange(it->aggroRange);
		m->SetLeashRange(it->leashRange);
		m->SetTemplateId(it->templateId);

		// MonsterTemplate 스탯 적용
		if (templateData)
		{
			m->info.set_name(templateData->name);
			m->info.set_maxhp(templateData->maxHp);
			m->info.set_hp(templateData->maxHp);
			m->info.set_attack(templateData->attack);
			m->info.set_defence(templateData->defence);
		}

		AddObject(m);

		it = _respawnQueue.erase(it);
	}
}
