#include "pch.h"
#include "PartyManager.h"

PartyManager GPartyManager;

uint64 PartyManager::CreateParty(uint64 leaderId)
{
	if (IsInParty(leaderId))
		return 0;

	uint64 partyId = _partyIdGen++;

	Party party;
	party.partyId = partyId;
	party.leaderId = leaderId;
	party.memberIds.push_back(leaderId);

	_parties[partyId] = party;
	_playerToParty[leaderId] = partyId;

	cout << "[Party] Created party " << partyId << " (leader=" << leaderId << ")" << endl;
	return partyId;
}

void PartyManager::DisbandParty(uint64 partyId)
{
	auto it = _parties.find(partyId);
	if (it == _parties.end())
		return;

	for (uint64 memberId : it->second.memberIds)
		_playerToParty.erase(memberId);

	_parties.erase(it);

	cout << "[Party] Disbanded party " << partyId << endl;
}

bool PartyManager::AddMember(uint64 partyId, uint64 playerId)
{
	Party* party = GetParty(partyId);
	if (!party)
		return false;

	if ((int32)party->memberIds.size() >= MAX_PARTY_SIZE)
		return false;

	if (IsInParty(playerId))
		return false;

	party->memberIds.push_back(playerId);
	_playerToParty[playerId] = partyId;

	cout << "[Party] Player " << playerId << " joined party " << partyId << endl;
	return true;
}

void PartyManager::RemoveMember(uint64 partyId, uint64 playerId)
{
	Party* party = GetParty(partyId);
	if (!party)
		return;

	auto& members = party->memberIds;
	members.erase(remove(members.begin(), members.end(), playerId), members.end());
	party->memberNames.erase(playerId);
	_playerToParty.erase(playerId);

	cout << "[Party] Player " << playerId << " left party " << partyId << endl;

	if (members.size() <= 1)
	{
		// 1명 이하면 해산
		DisbandParty(partyId);
		return;
	}

	// 리더가 나갔으면 다음 멤버에게 위임
	if (party->leaderId == playerId)
	{
		party->leaderId = members[0];
		cout << "[Party] New leader: " << party->leaderId << endl;
	}
}

Party* PartyManager::GetParty(uint64 partyId)
{
	auto it = _parties.find(partyId);
	if (it == _parties.end())
		return nullptr;
	return &it->second;
}

uint64 PartyManager::GetPartyIdByPlayer(uint64 playerId)
{
	auto it = _playerToParty.find(playerId);
	if (it == _playerToParty.end())
		return 0;
	return it->second;
}

bool PartyManager::IsInParty(uint64 playerId)
{
	return _playerToParty.find(playerId) != _playerToParty.end();
}

bool PartyManager::IsLeader(uint64 playerId)
{
	uint64 partyId = GetPartyIdByPlayer(playerId);
	if (partyId == 0)
		return false;

	Party* party = GetParty(partyId);
	if (!party)
		return false;

	return party->leaderId == playerId;
}
