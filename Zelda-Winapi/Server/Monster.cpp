#include "pch.h"
#include "Monster.h"
#include "GameRoom.h"
#include "Player.h"

Monster::Monster()
{
	info.set_name("MonsterName");
	info.set_hp(50);
	info.set_maxhp(50);
	info.set_attack(7);
	info.set_defence(0);
}

Monster::~Monster()
{

}

void Monster::Update()
{
	//Super::Update();

	switch (info.state())
	{
	case IDLE:
		UpdateIdle();
		break;
	case MOVE:
		UpdateMove();
		break;
	case SKILL:
		UpdateSkill();
		break;
	}
}

void Monster::UpdateIdle()
{
	if (room == nullptr)
		return;

	if (_homePos.x == -1 && _homePos.y == -1)
		_homePos = GetCellPos();

	Vec2Int myPos = GetCellPos();

	{
		Vec2Int dh = myPos - _homePos;
		int32 distHome = abs(dh.x) + abs(dh.y);
		if (distHome > _leashRange)
		{
			_target.reset();

			vector<Vec2Int> path;
			if (room->FindPath(myPos, _homePos, OUT path, 20))
			{
				if (path.size() > 1)
				{
					Vec2Int nextPos = path[1];
					if (room->CanGo(nextPos))
					{
						SetDir(GetLookAtDir(nextPos));
						SetCellPos(nextPos);
						_waitUntil = GetTickCount64() + 300;
						SetState(MOVE, true);
					}
				}
			}
			return;
		}
	}

	// Find/Refresh target
	PlayerRef target = _target.lock();
	PlayerRef nearest = room->FindClosestPlayer(myPos);

	if (nearest)
	{
		if (target == nullptr)
		{
			target = nearest;
			_target = nearest;
		}
		else
		{
			Vec2Int dtCur = target->GetCellPos() - myPos;
			Vec2Int dtNew = nearest->GetCellPos() - myPos;
			int32 distCur = abs(dtCur.x) + abs(dtCur.y);
			int32 distNew = abs(dtNew.x) + abs(dtNew.y);

			// retarget
			if (distNew + 1 < distCur)
			{
				target = nearest;
				_target = nearest;
			}
		}
	}

	if (target == nullptr)
	{
		if (myPos != _homePos)
		{
			vector<Vec2Int> path;
			if (room->FindPath(myPos, _homePos, OUT path, 20))
			{
				if (path.size() > 1)
				{
					Vec2Int nextPos = path[1];
					if (room->CanGo(nextPos))
					{
						SetDir(GetLookAtDir(nextPos));
						SetCellPos(nextPos);
						_waitUntil = GetTickCount64() + 300;
						SetState(MOVE, true);
					}
				}
			}
		}
		return;
	}

	Vec2Int dt = target->GetCellPos() - myPos;
	int32 distTarget = abs(dt.x) + abs(dt.y);
	if (distTarget > _aggroRange)
	{
		_target.reset();

		if (myPos != _homePos)
		{
			vector<Vec2Int> path;
			if (room->FindPath(myPos, _homePos, OUT path, 20))
			{
				if (path.size() > 1)
				{
					Vec2Int nextPos = path[1];
					if (room->CanGo(nextPos))
					{
						SetDir(GetLookAtDir(nextPos));
						SetCellPos(nextPos);
						_waitUntil = GetTickCount64() + 300;
						SetState(MOVE, true);
					}
				}
			}
		}
		return;
	}

	if (distTarget == 1)
	{
		SetDir(GetLookAtDir(target->GetCellPos()));
		SetState(SKILL, true);
		_waitUntil = GetTickCount64() + 1000;

		// 데미지 처리
		int32 damage = max(1, info.attack() - target->info.defence());
		target->OnDamaged(damage);

		{
			Protocol::S_Damaged dmgPkt;
			dmgPkt.set_attackerid(info.objectid());
			dmgPkt.set_targetid(target->info.objectid());
			dmgPkt.set_damage(damage);
			dmgPkt.set_newhp(target->info.hp());

			SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Damaged(dmgPkt);
			room->Broadcast(sendBuffer);
		}

		return;
	}

	vector<Vec2Int> path;
	if (room->FindPath(myPos, target->GetCellPos(), OUT path, 20))
	{
		if (path.size() > 1)
		{
			Vec2Int nextPos = path[1];

			Vec2Int dh2 = nextPos - _homePos;
			int32 distHome2 = abs(dh2.x) + abs(dh2.y);
			if (distHome2 > _leashRange)
			{
				_target.reset();
				return;
			}

			if (room->CanGo(nextPos))
			{
				SetDir(GetLookAtDir(nextPos));
				SetCellPos(nextPos);
				_waitUntil = GetTickCount64() + 1000;
				SetState(MOVE, true);
			}
		}
	}
}

void Monster::UpdateMove()
{
	uint64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	SetState(IDLE);
}

void Monster::UpdateSkill()
{
	uint64 now = GetTickCount64();

	if (_waitUntil > now)
		return;

	SetState(IDLE);
}
