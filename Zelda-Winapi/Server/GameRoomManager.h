#pragma once

class GameRoom;

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

    GameRoomRef CreateRoom();
    GameRoomRef FindRoom(RoomId roomId);
    void RemoveRoom(RoomId roomId);

    GameRoomRef GetDefaultRoom() const { return _defaultRoom.lock(); }

private:
    atomic<RoomId> _roomIdGenerator = 1;
    unordered_map<RoomId, GameRoomRef> _rooms;
    weak_ptr<GameRoom> _defaultRoom;
};

extern GameRoomManager GRoomManager;
