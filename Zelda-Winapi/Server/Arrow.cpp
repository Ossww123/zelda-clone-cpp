#include "pch.h"
#include "Arrow.h"
#include "GameRoom.h"
#include "Creature.h"
#include "Monster.h"

Arrow::Arrow()
{
	info.set_state(IDLE);
}

Arrow::~Arrow()
{
}

void Arrow::Update()
{
	if (room == nullptr)
		return;

	const uint64 now = GetTickCount64();

	if (_spawned == false)
	{
		_spawned = true;
		_expireTick = now + _lifeTimeMs;
		_waitUntil = now;

		if (room->IsBlockedByWall(GetCellPos()))
		{
			room->RemoveObject(info.objectid());
			return;
		}
	}

	if (now >= _expireTick)
	{
		room->RemoveObject(info.objectid());
		return;
	}

	switch (info.state())
	{
	case IDLE:
		UpdateIdle();
		break;
	case MOVE:
		UpdateMove();
		break;
	default:
		info.set_state(IDLE);
		BroadcastMove();
		break;
	}
}

void Arrow::UpdateIdle()
{
	if (room == nullptr)
		return;

	const uint64 now = GetTickCount64();
	if (_waitUntil > now)
		return;

	Vec2Int deltaXY[4] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
	Vec2Int nextPos = GetCellPos() + deltaXY[info.dir()];

	// 타일 막힘 체크
	if (room->IsBlockedByWall(nextPos))
	{
		room->RemoveObject(info.objectid());
		return;
	}

	// 충돌 체크(몬스터)
	MonsterRef target = room->GetMonsterAt(nextPos);
	if (target)
	{
		int32 damage = 0;

		GameObjectRef ownerObj = room->FindObject(_ownerId);
		CreatureRef attacker = std::dynamic_pointer_cast<Creature>(ownerObj);

		if (attacker && target->OnDamaged(attacker, damage))
		{
			Protocol::S_Damaged dmgPkt;
			dmgPkt.set_attackerid(attacker->info.objectid());
			dmgPkt.set_targetid(target->info.objectid());
			dmgPkt.set_damage(damage);
			dmgPkt.set_newhp(target->info.hp());

			SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Damaged(dmgPkt);
			room->Broadcast(sendBuffer);

			if (target->info.hp() == 0)
				room->RemoveObject(target->info.objectid());
		}

		room->RemoveObject(info.objectid());
		return;
	}

	// 이동
	SetCellPos(nextPos, true);
	SetState(MOVE, true);
	BroadcastMove();

	_waitUntil = now + _moveIntervalMs;
}

void Arrow::UpdateMove()
{
	const uint64 now = GetTickCount64();
	if (_waitUntil > now)
		return;

	SetState(IDLE, true);
	BroadcastMove();
}