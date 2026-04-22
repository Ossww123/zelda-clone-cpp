#pragma once

class GameSession;

class GameSessionManager
{
public:
	void Add(GameSessionRef session);
	void Remove(GameSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);
	void SendToPlayerIds(const vector<uint64>& playerIds, SendBufferRef sendBuffer);
	GameSessionRef FindByPlayerName(const string& name);

private:
	USE_LOCK;
	set<GameSessionRef> _sessions;
};

extern GameSessionManager GSessionManager;
