#pragma once

class GameRoom;
using RoomKey = std::pair<FieldId, int32>;

class GameRoomManager
{
public:
    using RoomId = uint64;
    using GameRoomRef = shared_ptr<GameRoom>;

public:
    GameRoomManager();
    ~GameRoomManager();

    void Init();

    void Update();

    GameRoomRef GetStaticRoom(FieldId field, int32 channel);
    uint64 CreateDungeonInstance();
    GameRoomRef GetDungeonInstance(uint64 instanceId);
    void RemoveDungeonInstance(uint64 instanceId);
    void RequestRemoveDungeonInstance(uint64 instanceId);

private:
    map<RoomKey, GameRoomRef> _staticRooms;
    std::unordered_map<uint64, GameRoomRef> _dungeonInstances;
    atomic<uint64> _instanceIdGen = 1;
    vector<uint64> _pendingRemoveDungeon;
};

extern GameRoomManager GRoomManager;
