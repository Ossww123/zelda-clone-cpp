#pragma once

#include "Player.h"

struct InventorySlot
{
	int32 itemId = 0;
	int32 count = 0;
};

struct PartyMemberData
{
	uint64 playerId = 0;
	wstring name;
	int32 level = 0;
	int32 hp = 0;
	int32 maxHp = 0;
	bool isLeader = false;
};

class MyPlayer : public Player
{
	using Super = Player;

public:
	MyPlayer ( );
	virtual ~MyPlayer ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:
	void TickInput ( );
	void TryMove ( );
	void TrySkill ( );

	virtual void TickIdle ( ) override;
	virtual void TickMove ( ) override;
	virtual void TickSkill ( ) override;

public:
	void OnServerTurnAck ( ) { _movePending = false; _turnGraceLeft = TURN_GRACE; }
	void OnServerMoveEndAck ( ) { _movePending = false; }
	void OnServerAttackAck ( ) { _attackPending = false; }
	void OnServerMoveResult ( const Protocol::ObjectInfo& info );

	void ApplyInventoryData ( const Protocol::S_InventoryData& pkt );
	void ApplyAddItem ( int32 slot, int32 itemId, int32 count );
	void ApplyEquipItem ( const Protocol::S_EquipItem& pkt );
	void ApplyUnequipItem ( const Protocol::S_UnequipItem& pkt );
	void ApplyUseItem ( const Protocol::S_UseItem& pkt );

public:
	// 인벤토리
	static const int32 INVENTORY_SIZE = 27;
	InventorySlot _storage[INVENTORY_SIZE];
	InventorySlot _equipWeapon;
	InventorySlot _equipArmor;
	InventorySlot _equipPotion;

	// 파티
	vector<PartyMemberData> _partyMembers;
	uint64 _pendingInviteFrom = 0;
	wstring _pendingInviterName;

private:
	InventorySlot* GetEquipSlot ( int32 equipType );

	bool _keyPressed = false;
	Protocol::DIR_TYPE _wantedDir = DIR_DOWN;
	bool _movePending = false;
	float _turnGraceLeft = 0.f;
	static constexpr float TURN_GRACE = 0.05f;

	bool  _attackPending = false;
	float _attackPendingStart = 0.f;
	float _attackCooldownUntil = 0.f;

	bool _blockedHold = false;
	Protocol::DIR_TYPE _blockedDir = DIR_DOWN;
	Vec2Int _lastMoveFrom{ 0, 0 };
	Protocol::DIR_TYPE _lastMoveDir = DIR_DOWN;
	bool _hasLastMoveRequest = false;
};

