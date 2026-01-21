#pragma once

class GameRoom;

class IRoomLogic
{
public:
    virtual ~IRoomLogic() = default;

    virtual void OnInit(GameRoom& room) {}
    virtual void OnUpdate(GameRoom& room) {}

    virtual void OnEnterRoom(GameRoom& room, GameSessionRef session) {}
    virtual void OnLeaveRoom(GameRoom& room, GameSessionRef session) {}

    // 오브젝트 제거 직전에(몬스터 죽음 리스폰 예약 같은 처리)
    virtual void OnBeforeRemoveObject(GameRoom& room, GameObjectRef obj) {}
};
