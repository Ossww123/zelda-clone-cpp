#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
#include "GameRoomManager.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));

	// 로그인 패킷

	Send(ServerPacketHandler::Make_S_EnterGame());

	// 게임 입장
	GameRoomRef room = GRoomManager.GetDefaultRoom();
	if (room)
		room->EnterRoom(GetSessionRef());
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	// 게임 나가기
	GameRoomRef room = GRoomManager.GetDefaultRoom();
	if (room)
		room->LeaveRoom(GetSessionRef());
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerPacketHandler::HandlePacket(static_pointer_cast<GameSession>(shared_from_this()), buffer, len);
}


void GameSession::OnSend(int32 len)
{
	// cout << "OnSend Len = " << len << endl;
}