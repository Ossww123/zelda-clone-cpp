#pragma once

struct Party
{
	uint64 partyId = 0;
	uint64 leaderId = 0;
	vector<uint64> memberIds;
	unordered_map<uint64, string> memberNames;
};

class PartyManager
{
public:
	uint64 CreateParty(uint64 leaderId);
	void DisbandParty(uint64 partyId);

	bool AddMember(uint64 partyId, uint64 playerId);
	void RemoveMember(uint64 partyId, uint64 playerId);

	Party* GetParty(uint64 partyId);
	uint64 GetPartyIdByPlayer(uint64 playerId);

	bool IsInParty(uint64 playerId);
	bool IsLeader(uint64 playerId);

	static const int32 MAX_PARTY_SIZE = 4;

private:
	unordered_map<uint64, Party> _parties;
	unordered_map<uint64, uint64> _playerToParty;
	atomic<uint64> _partyIdGen = 1;
};

extern PartyManager GPartyManager;
