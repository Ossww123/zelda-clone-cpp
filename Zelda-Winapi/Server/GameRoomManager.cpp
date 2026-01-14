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
    GameRoomRef room = CreateRoom();
    room->Init();
    _defaultRoom = room;
}

void GameRoomManager::Update()
{
    for (auto& [id, room] : _rooms)
    {
        room->Update();
    }
}

GameRoomManager::GameRoomRef GameRoomManager::CreateRoom()
{
    RoomId id = _roomIdGenerator++;

    GameRoomRef room = make_shared<GameRoom>();
    room->SetRoomId(id);
    _rooms.emplace(id, room);

    return room;
}

GameRoomManager::GameRoomRef GameRoomManager::FindRoom(RoomId roomId)
{
    auto it = _rooms.find(roomId);
    if (it == _rooms.end())
        return nullptr;

    return it->second;
}

void GameRoomManager::RemoveRoom(RoomId roomId)
{
    _rooms.erase(roomId);
}
