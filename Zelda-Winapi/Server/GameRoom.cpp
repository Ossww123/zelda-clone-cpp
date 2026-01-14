#include "pch.h"
#include "GameRoom.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "Arrow.h"
#include "GameSession.h"

GameRoom::GameRoom()
{
}

GameRoom::~GameRoom()
{
}

void GameRoom::Init()
{
	MonsterRef monster = GameObject::CreateMonster();
	monster->info.set_posx(8);
	monster->info.set_posy(8);
	AddObject(monster);

	_tilemap.LoadFile(L"../Resources/Tilemap/Tilemap_01.txt");
}

void GameRoom::Update()
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
}

void GameRoom::EnterRoom(GameSessionRef session)
{
	PlayerRef player = GameObject::CreatePlayer();

	// 서로의 존재를 연결/
	session->gameRoom = GetRoomRef();
	session->player = player;
	player->session = session;

	// TEMP
	player->info.set_posx(5);
	player->info.set_posy(5);

	// 입장한 클라에게 정보를 보내주기
	{
		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_MyPlayer(player->info);
		session->Send(sendBuffer);
	}
	// 모든 오브젝트 정보 전송
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

void GameRoom::Handle_C_Move(GameSessionRef session, Protocol::C_Move& pkt)
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

void GameRoom::Handle_C_Attack(GameSessionRef session, Protocol::C_Attack& pkt)
{
	PlayerRef attacker = session->player.lock();
	if (!attacker)
		return;

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
	default:
		break;
	}
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

	// 신규 오브젝트 정보 전송
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

	// 오브젝트 삭제 전송
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

	// 초기값
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
		// 제일 좋은 후보를 찾는다
		PQNode node = pq.top();
		pq.pop();

		// 더 짧은 경로를 뒤늦게 찾았다면 스킵
		if (best[node.pos] < node.cost)
			continue;

		// 목적지에 도착했으면 바로 종료
		if (node.pos == dest)
		{
			found = true;
			break;
		}

		// 방문
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
				// 다른 경로에서 더 빠른 길을 찾았으면 스킵
				if (bestValue <= cost)
					continue;
			}

			// 예약 진행
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

			// 동점이라면, 최초 위치에서 가장 덜 이동하는 쪽으로
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

		// 시작점
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

	// 몇 번 시도?
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
