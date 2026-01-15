#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
#include "GameRoomManager.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(GetSessionRef());

	// 로그인 패킷

	Send(ServerPacketHandler::Make_S_EnterGame());

	// 게임 입장
	GameRoomRef room = GRoomManager.GetDefaultRoom();
	if (room)
	{
		GameSessionRef self = GetSessionRef();
		room->PushJob([room, self]()
			{
				room->EnterRoom(self);
			});
	}
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(GetSessionRef());

	// 게임 나가기
	GameRoomRef room = gameRoom.lock();
	if (room)
	{
		GameSessionRef self = GetSessionRef();
		room->PushJob([room, self]()
			{
				room->LeaveRoom(self);
			});
	}
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerPacketHandler::HandlePacket(static_pointer_cast<GameSession>(shared_from_this()), buffer, len);
}


void GameSession::OnSend(int32 len)
{
	// cout << "OnSend Len = " << len << endl;
}