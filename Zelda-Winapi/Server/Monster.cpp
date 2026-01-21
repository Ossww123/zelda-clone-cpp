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

	// home에서 너무 멀어졌으면
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

	// Find Player
	if (_target.lock() == nullptr)
		_target = room->FindClosestPlayer(myPos);

	PlayerRef target = _target.lock();

	// target 이 없을 때 돌아가기
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

	// 타겟이 추적 범위 밖일 때
	Vec2Int dt = target->GetCellPos() - myPos;
	int32 distTarget = abs(dt.x) + abs(dt.y);
	if (distTarget > _aggroRange)
	{
		_target.reset();

		// home으로 복귀
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

	// 근접 시 공격
	if (distTarget == 1)
	{
		SetDir(GetLookAtDir(target->GetCellPos()));
		SetState(SKILL, true);
		_waitUntil = GetTickCount64() + 1000;
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
