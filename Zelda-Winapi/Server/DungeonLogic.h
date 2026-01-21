#pragma once
#include "IRoomLogic.h"

class DungeonLogic : public IRoomLogic
{
public:
    virtual void OnInit(GameRoom& room) override;
    virtual void OnUpdate(GameRoom& room) override;
    virtual void OnLeaveRoom(GameRoom& room, GameSessionRef session) override;
    virtual void OnBeforeRemoveObject(GameRoom& room, GameObjectRef obj) override;

private:
    void SpawnDungeonMonsters(GameRoom& room);

    struct RespawnRequest
    {
        uint64 when;
        Vec2Int homePos;
    };

    vector<RespawnRequest> _respawnQueue;

    void ReserveMonsterRespawn(Vec2Int homePos);
    void ProcessRespawn(GameRoom& room);
};
