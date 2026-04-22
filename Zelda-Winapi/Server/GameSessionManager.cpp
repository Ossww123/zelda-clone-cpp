#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"
#include "Player.h"

GameSessionManager GSessionManager;

void GameSessionManager::Add(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void GameSessionManager::Remove(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		session->Send(sendBuffer);
	}
}

void GameSessionManager::SendToPlayerIds(const vector<uint64>& playerIds, SendBufferRef sendBuffer)
{
    WRITE_LOCK;
    for (GameSessionRef session : _sessions)
    {
        PlayerRef player = session->player.lock();
        if (!player)
            continue;
        for (uint64 id : playerIds)
        {
            if (player->info.objectid() == id)
            {
                session->Send(sendBuffer);
                break;
            }
        }
    }
}

GameSessionRef GameSessionManager::FindByPlayerName(const string& name)
{
    WRITE_LOCK;
    for (GameSessionRef session : _sessions)
    {
        PlayerRef player = session->player.lock();
        if (player && player->info.name() == name)
            return session;
    }
    return nullptr;
}
