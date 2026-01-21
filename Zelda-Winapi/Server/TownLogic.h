#pragma once
#include "IRoomLogic.h"

class TownLogic : public IRoomLogic
{
public:
    virtual void OnInit(GameRoom& room) override {};
    virtual void OnUpdate(GameRoom& room) override {};
    virtual void OnLeaveRoom(GameRoom& room, GameSessionRef session) override {};
    virtual void OnBeforeRemoveObject(GameRoom& room, GameObjectRef obj) override {};

};
