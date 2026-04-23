#pragma once

class GameRoom;

class CombatSystem
{
public:
	CombatSystem(GameRoom& room);

	void HandleAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void DistributeExp(PlayerRef killer, MonsterRef monster);
	void ProcessMonsterDrop(PlayerRef killer, MonsterRef monster);

private:
	void HandleSwordAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void HandleBowAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void HandleStaffAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void BroadcastAttack(PlayerRef attacker, const Protocol::C_Attack& pkt);
	void BroadcastDamaged(PlayerRef attacker, CreatureRef target, int32 damage);

	GameRoom& _room;
};
