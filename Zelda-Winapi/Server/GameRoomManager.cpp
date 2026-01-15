#include "pch.h"
#include "GameRoomManager.h"
#include "GameRoom.h"

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
        room->LoadMap(L"../Resources/Tilemap/Tilemap_01.txt");
        room->Init();

        _staticRooms[{FieldId::Town, 1}] = room;
    }

    {
        GameRoomRef room = make_shared<GameRoom>();
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
