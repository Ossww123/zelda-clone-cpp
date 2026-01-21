#include "pch.h"
#include "DungeonLogic.h"
#include "GameRoom.h"
#include "Monster.h"
#include "GameRoomManager.h"

void DungeonLogic::OnInit(GameRoom& room)
{
    SpawnDungeonMonsters(room);
}

void DungeonLogic::OnUpdate(GameRoom& room)
{
    ProcessRespawn(room);
}

void DungeonLogic::OnLeaveRoom(GameRoom& room, GameSessionRef session)
{
    if (room.GetPlayerCount() == 0)
    {
        uint64 instanceId = room.GetInstanceId();
        if (instanceId != 0)
            GRoomManager.RequestRemoveDungeonInstance(instanceId);
    }
}

void DungeonLogic::OnBeforeRemoveObject(GameRoom& room, GameObjectRef obj)
{
    if (obj->info.objecttype() != Protocol::OBJECT_TYPE_MONSTER)
        return;

    MonsterRef monster = static_pointer_cast<Monster>(obj);
    ReserveMonsterRespawn(monster->GetHomePos());
}

void DungeonLogic::SpawnDungeonMonsters(GameRoom& room)
{
    Vec2Int anchorUR = { 30, 5 };
    Vec2Int anchorDR = { 30, 25 };
    Vec2Int anchorDL = { 5, 25 };

    Vec2Int offsets[3] = { {0,0}, {1,0}, {0,1} };

    auto spawnGroup = [&](Vec2Int anchor)
        {
            for (int i = 0; i < 3; i++)
            {
                Vec2Int pos = anchor + offsets[i];

                if (room.CanGo(pos) == false)
                    pos = room.GetRandomEmptyCellPos();

                MonsterRef m = GameObject::CreateMonster();
                m->SetHomePos(pos);
                m->SetCellPos(pos);

                room.AddObject(m);
            }
        };

    spawnGroup(anchorUR);
    spawnGroup(anchorDR);
    spawnGroup(anchorDL);
}

void DungeonLogic::ReserveMonsterRespawn(Vec2Int homePos)
{
    RespawnRequest req;
    req.when = GetTickCount64() + 10000;
    req.homePos = homePos;
    _respawnQueue.push_back(req);
}

void DungeonLogic::ProcessRespawn(GameRoom& room)
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

        if (room.CanGo(pos) == false)
            pos = room.GetRandomEmptyCellPos();

        MonsterRef m = GameObject::CreateMonster();
        m->SetHomePos(pos);
        m->SetCellPos(pos);
        room.AddObject(m);

        it = _respawnQueue.erase(it);
    }
}
