#include "pch.h"
#include "CombatSystem.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"
#include "Arrow.h"
#include "RoomDataManager.h"
#include "PartyManager.h"

CombatSystem::CombatSystem(GameRoom& room) : _room(room)
{
}

void CombatSystem::HandleAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	BroadcastAttack(attacker, pkt);

	switch (pkt.weapontype())
	{
	case Protocol::WEAPON_TYPE_SWORD:
		HandleSwordAttack(attacker, pkt);
		break;
	case Protocol::WEAPON_TYPE_BOW:
		HandleBowAttack(attacker, pkt);
		break;
	case Protocol::WEAPON_TYPE_STAFF:
		HandleStaffAttack(attacker, pkt);
		break;
	default:
		break;
	}
}

void CombatSystem::HandleSwordAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	Vec2Int frontPos = attacker->GetFrontCellPos();
	GameObjectRef obj = _room.GetGameObjectAt(frontPos);
	CreatureRef target = dynamic_pointer_cast<Creature>(obj);

	if (!target)
		return;

	int32 damage = max(1, attacker->info.attack() - target->info.defence());
	target->OnDamaged(damage);

	BroadcastDamaged(attacker, target, damage);

	if (target->info.hp() == 0)
	{
		MonsterRef monster = dynamic_pointer_cast<Monster>(target);
		if (monster)
		{
			DistributeExp(attacker, monster);
			ProcessMonsterDrop(attacker, monster);
		}
		_room.RemoveObject(target->info.objectid());
	}
}

void CombatSystem::HandleBowAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	ArrowRef arrow = GameObject::CreateArrow();
	arrow->info.set_dir(pkt.dir());

	Vec2Int start = attacker->GetCellPos();
	arrow->info.set_posx(start.x);
	arrow->info.set_posy(start.y);

	arrow->SetOwner(attacker->info.objectid());

	_room.AddObject(arrow);
}

void CombatSystem::HandleStaffAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	Vec2Int pos = attacker->GetCellPos();

	Vec2Int forward = { 0, 0 };
	switch (pkt.dir())
	{
	case Protocol::DIR_TYPE_UP:    forward = { 0, -2 }; break;
	case Protocol::DIR_TYPE_DOWN:  forward = { 0,  2 }; break;
	case Protocol::DIR_TYPE_LEFT:  forward = { -2, 0 }; break;
	case Protocol::DIR_TYPE_RIGHT: forward = { 2,  0 }; break;
	default: break;
	}

	Vec2Int center = pos + forward;

	vector<pair<uint64, MonsterRef>> deadTargets;

	for (int32 dy = -1; dy <= 1; dy++)
	{
		for (int32 dx = -1; dx <= 1; dx++)
		{
			Vec2Int cell = { center.x + dx, center.y + dy };

			MonsterRef target = _room.GetMonsterAt(cell);
			if (!target)
				continue;

			int32 damage = max(1, static_cast<int32>((attacker->info.attack() - target->info.defence()) * 0.5f));
			target->OnDamaged(damage);

			BroadcastDamaged(attacker, target, damage);

			if (target->info.hp() == 0)
				deadTargets.push_back({ target->info.objectid(), target });
		}
	}

	for (auto& [id, monster] : deadTargets)
	{
		DistributeExp(attacker, monster);
		ProcessMonsterDrop(attacker, monster);
		_room.RemoveObject(id);
	}
}

void CombatSystem::BroadcastAttack(PlayerRef attacker, const Protocol::C_Attack& pkt)
{
	Protocol::S_Attack atk;
	atk.set_attackerid(attacker->info.objectid());
	atk.set_dir(pkt.dir());
	atk.set_weapontype(pkt.weapontype());

	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Attack(atk);
	_room.Broadcast(sendBuffer);
}

void CombatSystem::BroadcastDamaged(PlayerRef attacker, CreatureRef target, int32 damage)
{
	Protocol::S_Damaged dmgPkt;
	dmgPkt.set_attackerid(attacker->info.objectid());
	dmgPkt.set_targetid(target->info.objectid());
	dmgPkt.set_damage(damage);
	dmgPkt.set_newhp(target->info.hp());

	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Damaged(dmgPkt);
	_room.Broadcast(sendBuffer);
}

void CombatSystem::DistributeExp(PlayerRef killer, MonsterRef monster)
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

	Party* party = GPartyManager.GetParty(partyId);
	if (!party)
	{
		killer->GainExp(exp);
		return;
	}

	const auto& players = _room.GetPlayers();
	vector<PlayerRef> nearbyMembers;
	for (uint64 memberId : party->memberIds)
	{
		auto it = players.find(memberId);
		if (it != players.end())
			nearbyMembers.push_back(it->second);
	}

	if (nearbyMembers.empty())
		return;

	int32 share = max(1, exp / (int32)nearbyMembers.size());
	for (auto& member : nearbyMembers)
		member->GainExp(share);
}

void CombatSystem::ProcessMonsterDrop(PlayerRef killer, MonsterRef monster)
{
	if (!killer || !monster)
		return;

	const vector<MonsterDropData>* drops = GRoomDataManager.GetMonsterDrops(monster->GetTemplateId());
	if (!drops)
		return;

	for (const auto& drop : *drops)
	{
		if (rand() % 100 < drop.dropRate)
			killer->AddItem(drop.itemId, 1);
	}
}
