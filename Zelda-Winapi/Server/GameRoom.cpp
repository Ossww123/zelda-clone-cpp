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
#include "PartyManager.h"

namespace
{
	const char* ToWeaponName(Protocol::WEAPON_TYPE type)
	{
		switch (type)
		{
		case Protocol::WEAPON_TYPE_SWORD: return "Sword";
		case Protocol::WEAPON_TYPE_BOW: return "Bow";
		case Protocol::WEAPON_TYPE_STAFF: return "Staff";
		default: return "Unknown";
		}
	}

	const char* ToDirName(Protocol::DIR_TYPE dir)
	{
		switch (dir)
		{
		case Protocol::DIR_TYPE_UP: return "Up";
		case Protocol::DIR_TYPE_DOWN: return "Down";
		case Protocol::DIR_TYPE_LEFT: return "Left";
		case Protocol::DIR_TYPE_RIGHT: return "Right";
		default: return "Unknown";
		}
	}
}
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

	// ??쇰㏊ 濡쒕뱶
	LoadMap(_config->tilemapPath.c_str());

	// 紐ъ뒪???ㅽ룿 (?ㅼ젙???곕씪)
	if (_config->monsterSpawnEnabled && _spawnConfig)
	{
		SpawnMonstersFromData();
	}

	cout << "[GameRoom] Initialized room: " << roomId << " (SkillEnabled=" << _config->skillEnabled
		<< ", MonsterSpawn=" << _config->monsterSpawnEnabled << ")" << endl;
}

void GameRoom::Init()
{
	// ?덇굅??濡쒖쭅 ?쒓굅??
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

	// ?덈Т 諛?몄쑝硫?由ъ뀑
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

	// ?곗씠??湲곕컲 由ъ뒪??泥섎━
	if (ShouldSpawnMonsters())
	{
		ProcessRespawnFromData();
	}
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

	// ?ㅽ룿 ?꾩튂濡?由ъ뀑
	player->info.set_posx(5);
	player->info.set_posy(5);
	player->info.set_state(IDLE);
	player->StopMove();

	{
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_MyPlayer(player->info);
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

	// ?뚰떚 ?뚯냽?대㈃ 媛숈? 諛⑹쓽 紐⑤뱺 ?뚰떚?먯뿉寃??뚰떚 ?뺣낫 媛깆떊
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
					// ?대쫫 + (Away) ?뺥깭
					auto nameIt = party->memberNames.find(memberId);
					string name = (nameIt != party->memberNames.end()) ? nameIt->second : "Unknown";
					m->set_name(name + "(Away)");
				}
			}
			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(partyPkt, S_PartyUpdate);

			// ??諛⑹뿉 ?덈뒗 紐⑤뱺 ?뚰떚?먯뿉寃??꾩넚
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

	// 鍮??섏쟾 ?몄뒪?댁뒪 ?먮룞 ?쒓굅
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
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(player->info);
		Broadcast(sendBuffer);
		return;
	}

	player->info.set_state(MOVE);
	player->info.set_posx(nextPos.x);
	player->info.set_posy(nextPos.y);

	// TEMP : 怨좎젙 ?대룞 ?쒓컙 
	constexpr uint64 kMoveDurationMs = 75;
	uint64 now = GetTickCount64();
	player->StartMove(now, kMoveDurationMs);

	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(player->info);
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

	// ?대? 媛숈? 諛⑺뼢
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

	//cout << "[CombatAuth][Recv] room=" << _roomIdStr
	//	<< " attacker=" << attacker->info.objectid()
	//	<< " weapon=" << ToWeaponName(pkt.weapontype())
	//	<< " dir=" << ToDirName(pkt.dir())
	//	<< endl;

	//if (!CanUseSkill())
	//{
	//	cout << "[CombatAuth][Reject] room=" << _roomIdStr
	//		<< " attacker=" << attacker->info.objectid()
	//		<< " reason=SkillDisabledInThisRoom" << endl;
	//	return;
	//}

	//cout << "[CombatAuth][Accept] attacker=" << attacker->info.objectid()
	//	<< " server_authoritative=true" << endl;

	BroadcastAttack(attacker, pkt);

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

	// 占신깍옙 占쏙옙占쏙옙占쏙옙트 占쏙옙占쏙옙 占쏙옙占쏙옙
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

	// ?곗씠??湲곕컲 紐ъ뒪??由ъ뒪???덉빟
	if (gameObject->info.objecttype() == Protocol::OBJECT_TYPE_MONSTER && ShouldSpawnMonsters())
	{
		MonsterRef monster = static_pointer_cast<Monster>(gameObject);

		RespawnRequest req;
		req.when = GetTickCount64() + GetRespawnTime();
		req.homePos = monster->GetHomePos();
		req.templateId = monster->GetTemplateId();
		req.level = 1; // TODO: ?덈꺼 ?쒖뒪??援ы쁽 ??蹂寃?
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

	// 占쏙옙占쏙옙占쏙옙트 占쏙옙占쏙옙 占쏙옙占쏙옙
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

	// 占십기값
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
		// 占쏙옙占쏙옙 占쏙옙占쏙옙 占식븝옙占쏙옙 찾占승댐옙
		PQNode node = pq.top();
		pq.pop();

		// 占쏙옙 짧占쏙옙 占쏙옙罐占?占쌘늦곤옙 찾占쌀다몌옙 占쏙옙킵
		if (best[node.pos] < node.cost)
			continue;

		// 占쏙옙占쏙옙占쏙옙占쏙옙 占쏙옙占쏙옙占쏙옙占쏙옙占쏙옙 占쌕뤄옙 占쏙옙占쏙옙
		if (node.pos == dest)
		{
			found = true;
			break;
		}

		// 占썸문
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
				// 占쌕몌옙 占쏙옙恝占쏙옙占?占쏙옙 占쏙옙占쏙옙 占쏙옙占쏙옙 찾占쏙옙占쏙옙占쏙옙 占쏙옙킵
				if (bestValue <= cost)
					continue;
			}

			// 占쏙옙占쏙옙 占쏙옙占쏙옙
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

			// 占쏙옙占쏙옙占싱띰옙占? 占쏙옙占쏙옙 占쏙옙치占쏙옙占쏙옙 占쏙옙占쏙옙 占쏙옙 占싱듸옙占싹댐옙 占쏙옙占쏙옙占쏙옙
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

		// 占쏙옙占쏙옙占쏙옙
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

	// 占쏙옙 占쏙옙 占시듸옙?
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
	{
		/*cout << "[CombatAuth][Result] weapon=Sword attacker=" << attacker->info.objectid()
			<< " result=Miss reason=NoTargetAtFrontCell"
			<< " front=(" << frontPos.x << "," << frontPos.y << ")" << endl;*/
		return;
	}

	int32 beforeHp = target->info.hp();
	int32 damage = max(1, attacker->info.attack() - target->info.defence());
	target->OnDamaged(damage);
	int32 afterHp = target->info.hp();

	/*cout << "[CombatAuth][Result] weapon=Sword attacker=" << attacker->info.objectid()
		<< " target=" << target->info.objectid()
		<< " damage=" << damage
		<< " hp=" << beforeHp << "->" << afterHp << endl;*/

	BroadcastDamaged(attacker, target, damage);

	if (target->info.hp() == 0)
	{
		MonsterRef monster = dynamic_pointer_cast<Monster>(target);
		if (monster)
		{
			DistributeExp(attacker, monster);
			ProcessMonsterDrop(attacker, monster);
		}
		RemoveObject(target->info.objectid());
	}
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

	/*cout << "[CombatAuth][Result] weapon=Bow attacker=" << attacker->info.objectid()
		<< " result=SpawnProjectile projectile=" << arrow->info.objectid()
		<< " pos=(" << start.x << "," << start.y << ")"
		<< " dir=" << ToDirName(pkt.dir()) << endl;*/
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
		forward = { 0, -2 };
		break;
	case Protocol::DIR_TYPE_DOWN:
		forward = { 0,  2 };
		break;
	case Protocol::DIR_TYPE_LEFT:
		forward = { -2, 0 };
		break;
	case Protocol::DIR_TYPE_RIGHT:
		forward = { 2,  0 };
		break;
	default:
		break;
	}

	Vec2Int center = pos + forward;

	std::vector<std::pair<uint64, MonsterRef>> deadTargets;
	int32 hitCount = 0;
	int32 totalDamage = 0;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			Vec2Int cell = { center.x + dx, center.y + dy };

			MonsterRef target = GetMonsterAt(cell);
			if (!target)
				continue;

			int32 beforeHp = target->info.hp();
			int32 damage = max(1, static_cast<int32>((attacker->info.attack() - target->info.defence()) * 0.5f));
			target->OnDamaged(damage);
			int32 afterHp = target->info.hp();
			hitCount++;
			totalDamage += damage;

			/*cout << "[CombatAuth][Result] weapon=Staff attacker=" << attacker->info.objectid()
				<< " target=" << target->info.objectid()
				<< " damage=" << damage
				<< " hp=" << beforeHp << "->" << afterHp << endl;*/

			BroadcastDamaged(attacker, target, damage);

			if (target->info.hp() == 0)
				deadTargets.push_back({ target->info.objectid(), target });
		}
	}

	for (auto& [id, monster] : deadTargets)
	{
		DistributeExp(attacker, monster);
		ProcessMonsterDrop(attacker, monster);
		RemoveObject(id);
	}

	if (hitCount == 0)
	{
		/*cout << "[CombatAuth][Result] weapon=Staff attacker=" << attacker->info.objectid()
			<< " result=Miss areaCenter=(" << center.x << "," << center.y << ")" << endl;*/
	}
	else
	{
		/*cout << "[CombatAuth][Result] weapon=Staff attacker=" << attacker->info.objectid()
			<< " result=Hit targets=" << hitCount
			<< " totalDamage=" << totalDamage << endl;*/
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

// ?곗씠??湲곕컲 猷??ㅼ젙 議고쉶
bool GameRoom::CanUseSkill() const
{
	if (_config)
		return _config->skillEnabled;
	return true; // 湲곕낯媛? ?덉슜
}

bool GameRoom::ShouldSpawnMonsters() const
{
	if (_config)
		return _config->monsterSpawnEnabled;
	return false; // 湲곕낯媛? ?ㅽ룿 ?덊븿
}

uint32 GameRoom::GetRespawnTime() const
{
	if (_config)
		return _config->respawnTimeMs;
	return 10000; // 湲곕낯媛? 10珥?
}

// ?곗씠??湲곕컲 紐ъ뒪???ㅽ룿
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

			// MonsterTemplate?먯꽌 ?ㅽ꺈 濡쒕뱶
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

				// MonsterTemplate ?ㅽ꺈 ?곸슜
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

// ?곗씠??湲곕컲 由ъ뒪??泥섎━
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

		// MonsterTemplate?먯꽌 ?ㅽ꺈 濡쒕뱶
		const MonsterTemplateData* templateData = GRoomDataManager.GetMonsterTemplate(it->templateId);

		MonsterRef m = GameObject::CreateMonster();
		m->SetHomePos(pos);
		m->SetCellPos(pos);
		m->SetAggroRange(it->aggroRange);
		m->SetLeashRange(it->leashRange);
		m->SetTemplateId(it->templateId);

		// MonsterTemplate ?ㅽ꺈 ?곸슜
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

void GameRoom::DistributeExp(PlayerRef killer, MonsterRef monster)
{
	if (!killer || !monster)
		return;

	const MonsterTemplateData* templateData = GRoomDataManager.GetMonsterTemplate(monster->GetTemplateId());
	if (!templateData)
		return;

	int32 exp = templateData->exp;
	if (exp <= 0)
		return;

	uint64 killerId = killer->info.objectid();
	uint64 partyId = GPartyManager.GetPartyIdByPlayer(killerId);

	if (partyId == 0)
	{
		killer->GainExp(exp);
		return;
	}

	// ?뚰떚: 媛숈? 諛⑹뿉 ?덈뒗 ?뚰떚?먯뿉寃?洹좊벑 遺꾨같
	Party* party = GPartyManager.GetParty(partyId);
	if (!party)
	{
		killer->GainExp(exp);
		return;
	}

	vector<PlayerRef> nearbyMembers;
	for (uint64 memberId : party->memberIds)
	{
		auto it = _players.find(memberId);
		if (it != _players.end())
			nearbyMembers.push_back(it->second);
	}

	if (nearbyMembers.empty())
		return;

	int32 share = max(1, exp / (int32)nearbyMembers.size());
	for (auto& member : nearbyMembers)
		member->GainExp(share);
}

void GameRoom::ProcessMonsterDrop(PlayerRef killer, MonsterRef monster)
{
	if (!killer || !monster)
		return;

	const vector<MonsterDropData>* drops = GRoomDataManager.GetMonsterDrops(monster->GetTemplateId());
	if (!drops)
		return;

	for (const auto& drop : *drops)
	{
		if (rand() % 100 < drop.dropRate)
		{
			killer->AddItem(drop.itemId, 1);
		}
	}
}

