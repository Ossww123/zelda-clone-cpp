#include "pch.h"
#include "PartyManager.h"

PartyManager GPartyManager;

uint64 PartyManager::CreateParty(uint64 leaderId)
{
	WRITE_LOCK;
	if (_playerToParty.find(leaderId) != _playerToParty.end())
		return 0;

	uint64 partyId = _partyIdGen++;

	Party party;
	party.partyId = partyId;
	party.leaderId = leaderId;
	party.memberIds.push_back(leaderId);

	_parties[partyId] = party;
	_playerToParty[leaderId] = partyId;

	LOG_INFO("Party", "Created party %llu (leader=%llu)", partyId, leaderId);
	return partyId;
}

bool PartyManager::AddMember(uint64 partyId, uint64 playerId)
{
	WRITE_LOCK;
	auto it = _parties.find(partyId);
	if (it == _parties.end())
		return false;

	Party* party = &it->second;

	if ((int32)party->memberIds.size() >= MAX_PARTY_SIZE)
		return false;

	if (_playerToParty.find(playerId) != _playerToParty.end())
		return false;

	party->memberIds.push_back(playerId);
	_playerToParty[playerId] = partyId;

	LOG_INFO("Party", "Player %llu joined party %llu", playerId, partyId);
	return true;
}

void PartyManager::RemoveMember(uint64 partyId, uint64 playerId)
{
	WRITE_LOCK;
	auto it = _parties.find(partyId);
	if (it == _parties.end())
		return;

	Party* party = &it->second;
	auto& members = party->memberIds;
	members.erase(remove(members.begin(), members.end(), playerId), members.end());
	party->memberNames.erase(playerId);
	_playerToParty.erase(playerId);

	LOG_INFO("Party", "Player %llu left party %llu", playerId, partyId);

	if (members.size() <= 1)
	{
		DisbandParty(partyId);
		return;
	}

	if (party->leaderId == playerId)
	{
		party->leaderId = members[0];
		LOG_INFO("Party", "New leader: %llu", party->leaderId);
	}
}

Party* PartyManager::GetParty(uint64 partyId)
{
	WRITE_LOCK;
	auto it = _parties.find(partyId);
	if (it == _parties.end())
		return nullptr;
	return &it->second;
}

uint64 PartyManager::GetPartyIdByPlayer(uint64 playerId)
{
	WRITE_LOCK;
	auto it = _playerToParty.find(playerId);
	if (it == _playerToParty.end())
		return 0;
	return it->second;
}

bool PartyManager::IsInParty(uint64 playerId)
{
	WRITE_LOCK;
	return _playerToParty.find(playerId) != _playerToParty.end();
}

bool PartyManager::IsLeader(uint64 playerId)
{
	WRITE_LOCK;
	auto ptIt = _playerToParty.find(playerId);
	if (ptIt == _playerToParty.end())
		return false;

	auto partyIt = _parties.find(ptIt->second);
	if (partyIt == _parties.end())
		return false;

	return partyIt->second.leaderId == playerId;
}

// private — WRITE_LOCK 없음, 항상 이미 락을 잡은 메서드에서 호출
void PartyManager::DisbandParty(uint64 partyId)
{
	auto it = _parties.find(partyId);
	if (it == _parties.end())
		return;

	for (uint64 memberId : it->second.memberIds)
		_playerToParty.erase(memberId);

	_parties.erase(it);

	LOG_INFO("Party", "Disbanded party %llu", partyId);
}
