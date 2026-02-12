#include "pch.h"
#include "GameRoomManager.h"
#include "GameRoom.h"
#include "RoomDataManager.h"

GameRoomManager GRoomManager;

GameRoomManager::GameRoomManager()
{
}

GameRoomManager::~GameRoomManager()
{
}

void GameRoomManager::Init()
{
    if (!GRoomDataManager.LoadAllData())
    {
        cout << "[GameRoomManager] ERROR: Failed to load room data!" << endl;
        return;
    }

    {
        GameRoomRef room = make_shared<GameRoom>();
        room->SetFieldId(FieldId::Town);
        room->SetChannel(1);
        room->SetInstanceId(0);
        room->InitFromConfig("town_1");
        room->Init();
        _staticRooms[{FieldId::Town, 1}] = room;
    }

    {
        GameRoomRef room = make_shared<GameRoom>();
        room->SetFieldId(FieldId::Town);
        room->SetChannel(2);
        room->SetInstanceId(0);
        room->InitFromConfig("town_2");
        room->Init();
        _staticRooms[{FieldId::Town, 2}] = room;
    }

    cout << "[GameRoomManager] Initialized " << _staticRooms.size() << " static rooms" << endl;
}

void GameRoomManager::Update(uint64 now)
{
    for (auto& kv : _staticRooms)
    {
        kv.second->Update(now);
    }

    for (auto& kv : _dungeonInstances)
    {
        kv.second->Update(now);
    }

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
    return CreateDungeonInstance("dungeon_forest");
}

uint64 GameRoomManager::CreateDungeonInstance(const string& roomId)
{
    uint64 instanceId = _instanceIdGen++;

    GameRoomRef room = make_shared<GameRoom>();
    room->SetFieldId(FieldId::Dungeon);
    room->SetChannel(0);
    room->SetInstanceId(instanceId);
    room->InitFromConfig(roomId);
    room->Init();

    _dungeonInstances[instanceId] = room;

    cout << "[GameRoomManager] Created dungeon instance: " << roomId << " (ID=" << instanceId << ")" << endl;

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
    auto it = _dungeonInstances.find(instanceId);
    if (it != _dungeonInstances.end())
    {
        _dungeonInstances.erase(instanceId);
        cout << "[GameRoomManager] Dungeon instance removed: " << instanceId << endl;
    }
    else
    {
        cout << "[GameRoomManager] WARNING: Dungeon instance not found: " << instanceId << endl;
    }
}

void GameRoomManager::RequestRemoveDungeonInstance(uint64 instanceId)
{
    _pendingRemoveDungeon.push_back(instanceId);
}
