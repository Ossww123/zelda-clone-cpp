#include "pch.h"
#include "SpawnSystem.h"
#include "GameRoom.h"
#include "Monster.h"
#include "RoomConfig.h"
#include "RoomDataManager.h"

SpawnSystem::SpawnSystem(GameRoom& room) : _room(room)
{
}

void SpawnSystem::Init(const RoomConfigData* config, const RoomSpawnConfig* spawnConfig)
{
	_config = config;
	_spawnConfig = spawnConfig;

	if (_config && _config->monsterSpawnEnabled && _spawnConfig)
		SpawnMonstersFromData();
}

void SpawnSystem::Tick()
{
	if (ShouldSpawn())
		ProcessRespawn();
}

bool SpawnSystem::ShouldSpawn() const
{
	return _config && _config->monsterSpawnEnabled;
}

void SpawnSystem::TryReserve(MonsterRef monster)
{
	if (!ShouldSpawn())
		return;

	uint32 respawnTimeMs = _config ? _config->respawnTimeMs : 10000;

	RespawnRequest req;
	req.when = GetTickCount64() + respawnTimeMs;
	req.homePos = monster->GetHomePos();
	req.templateId = monster->GetTemplateId();
	req.level = 1; // TODO: 서버 몬스터 레벨 구현 시 변경
	req.aggroRange = monster->GetAggroRange();
	req.leashRange = monster->GetLeashRange();

	_respawnQueue.push_back(req);
}

void SpawnSystem::SpawnMonstersFromData()
{
	if (!_spawnConfig)
	{
		LOG_ERROR("Room", "_spawnConfig is null! Cannot spawn monsters.");
		return;
	}

	LOG_INFO("Room", "Starting monster spawn. Spawn groups: %zu", _spawnConfig->spawns.size());

	for (const auto& spawnGroup : _spawnConfig->spawns)
	{
		LOG_INFO("Room", "Processing spawn group: %s at (%d, %d)",
			spawnGroup.groupId.c_str(), spawnGroup.anchor.x, spawnGroup.anchor.y);

		Vec2Int anchor = spawnGroup.anchor;
		int offsetIndex = 0;

		LOG_INFO("Room", "Monster types in this group: %zu", spawnGroup.monsters.size());

		for (const auto& monsterInfo : spawnGroup.monsters)
		{
			LOG_INFO("Room", "Monster templateId=%d, count=%d", monsterInfo.templateId, monsterInfo.count);

			const MonsterTemplateData* templateData = GRoomDataManager.GetMonsterTemplate(monsterInfo.templateId);
			if (!templateData)
			{
				LOG_ERROR("Room", "MonsterTemplate not found: %d", monsterInfo.templateId);
				continue;
			}

			for (int i = 0; i < monsterInfo.count; ++i)
			{
				if (offsetIndex >= (int)spawnGroup.offsets.size())
				{
					LOG_WARN("Room", "Not enough offsets for monster count in group: %s", spawnGroup.groupId.c_str());
					break;
				}

				Vec2Int pos = anchor + spawnGroup.offsets[offsetIndex];
				offsetIndex++;

				if (!_room.CanGo(pos))
					pos = _room.GetRandomEmptyCellPos();

				MonsterRef m = GameObject::CreateMonster();
				m->SetHomePos(pos);
				m->SetCellPos(pos);
				m->SetAggroRange(monsterInfo.aggroRange);
				m->SetLeashRange(monsterInfo.leashRange);
				m->SetTemplateId(monsterInfo.templateId);

				m->info.set_name(templateData->name);
				m->info.set_maxhp(templateData->maxHp);
				m->info.set_hp(templateData->maxHp);
				m->info.set_attack(templateData->attack);
				m->info.set_defence(templateData->defence);

				_room.AddObject(m);
			}
		}
	}

	LOG_INFO("Room", "Spawned monsters from data");
}

void SpawnSystem::ProcessRespawn()
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
		if (!_room.CanGo(pos))
			pos = _room.GetRandomEmptyCellPos();

		const MonsterTemplateData* templateData = GRoomDataManager.GetMonsterTemplate(it->templateId);

		MonsterRef m = GameObject::CreateMonster();
		m->SetHomePos(pos);
		m->SetCellPos(pos);
		m->SetAggroRange(it->aggroRange);
		m->SetLeashRange(it->leashRange);
		m->SetTemplateId(it->templateId);

		if (templateData)
		{
			m->info.set_name(templateData->name);
			m->info.set_maxhp(templateData->maxHp);
			m->info.set_hp(templateData->maxHp);
			m->info.set_attack(templateData->attack);
			m->info.set_defence(templateData->defence);
		}

		_room.AddObject(m);

		it = _respawnQueue.erase(it);
	}
}
