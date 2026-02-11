#pragma once

#include "Player.h"

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

	void SyncToServer ( );  // 미사용

public:
	void OnServerAck ( ) { _movePending = false; }
	void OnServerTurnAck ( ) { _movePending = false; _turnGraceLeft = TURN_GRACE; }
	void OnServerMoveEndAck ( ) { _movePending = false; }

private:
	bool _keyPressed = false;
	Protocol::DIR_TYPE _wantedDir = DIR_DOWN;
	bool _movePending = false;
	float _turnGraceLeft = 0.f;
	static constexpr float TURN_GRACE = 0.12f;
};

