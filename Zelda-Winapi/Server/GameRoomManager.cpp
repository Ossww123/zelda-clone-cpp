#include "pch.h"
#include "GameRoomManager.h"
#include "GameRoom.h"
#include "IRoomLogic.h"
#include "TownLogic.h"
#include "DungeonLogic.h"

GameRoomManager GRoomManager;

GameRoomManager::GameRoomManager()
{
}

GameRoomManager::~GameRoomManager()
{
}

void GameRoomManager::Init()
{
    {
        GameRoomRef room = make_shared<GameRoom>();
        room->SetFieldId(FieldId::Town);
        room->SetChannel(1);
        room->SetInstanceId(0);
        room->SetLogic(make_unique<TownLogic>());
        room->LoadMap(L"../Resources/Tilemap/Tilemap_01.txt");
        room->Init();
        _staticRooms[{FieldId::Town, 1}] = room;
    }

    {
        GameRoomRef room = make_shared<GameRoom>();
        room->SetFieldId(FieldId::Town);
        room->SetChannel(2);
        room->SetInstanceId(0);
        room->SetLogic(make_unique<TownLogic>());
        room->LoadMap(L"../Resources/Tilemap/Tilemap_01.txt");
        room->Init();
        _staticRooms[{FieldId::Town, 2}] = room;
    }
}

void GameRoomManager::Update()
{
    // 마을 룸 업데이트
    for (auto& kv : _staticRooms)
    {
        kv.second->Update();
    }

    // 던전 룸 업데이트
    for (auto& kv : _dungeonInstances)
    {
        kv.second->Update();
    }

    // 던전 삭제 (Update 마지막에)
    if (_pendingRemoveDungeon.empty() == false)
    {
        for (uint64 instanceId : _pendingRemoveDungeon)
        {
            RemoveDungeonInstance(instanceId);
        }
        _pendingRemoveDungeon.clear();
    }
}

GameRoomRef GameRoomManager::GetStaticRoom(FieldId field, int32 channel)
{
    auto it = _staticRooms.find({ field, channel });
    if (it == _staticRooms.end())
        return nullptr;

    return it->second;
}

uint64 GameRoomManager::CreateDungeonInstance()
{
    uint64 instanceId = _instanceIdGen++;

    GameRoomRef room = make_shared<GameRoom>();
    room->SetFieldId(FieldId::Dungeon);
    room->SetChannel(0);
    room->SetInstanceId(instanceId);
    room->SetLogic(make_unique<DungeonLogic>());
    room->LoadMap(L"../Resources/Tilemap/Tilemap_02.txt");
    room->Init();

    _dungeonInstances[instanceId] = room;
    return instanceId;
}

GameRoomRef GameRoomManager::GetDungeonInstance(uint64 instanceId)
{
    auto it = _dungeonInstances.find(instanceId);
    if (it == _dungeonInstances.end())
        return nullptr;

    return it->second;
}

void GameRoomManager::RemoveDungeonInstance(uint64 instanceId)
{
    _dungeonInstances.erase(instanceId);
}

void GameRoomManager::RequestRemoveDungeonInstance(uint64 instanceId)
{
    _pendingRemoveDungeon.push_back(instanceId);
}
