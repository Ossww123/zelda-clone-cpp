#include "pch.h"
#include "Player.h"
#include "RoomDataManager.h"
#include "GameRoom.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"

Player::Player()
{
	info.set_name("PlayerName");

	_level = 1;
	_exp = 0;

	const LevelData* data = GRoomDataManager.GetLevelData(1);
	if (data)
	{
		info.set_hp(data->maxHp);
		info.set_maxhp(data->maxHp);
		info.set_attack(data->attack);
		info.set_defence(data->defence);
	}
	else
	{
		info.set_hp(100);
		info.set_maxhp(100);
		info.set_attack(20);
		info.set_defence(5);
	}

	// PlayerExtra 세팅
	Protocol::PlayerExtra* extra = info.mutable_player();
	extra->set_level(_level);
	extra->set_exp(_exp);
	extra->set_maxexp(GetMaxExp());
}

Player::~Player()
{
}

void Player::Update()
{
	Super::Update();
}

int32 Player::GetMaxExp() const
{
	const LevelData* data = GRoomDataManager.GetLevelData(_level + 1);
	if (data)
		return data->requiredExp;
	return 0;
}

void Player::GainExp(int32 amount)
{
	if (amount <= 0)
		return;

	// 만렙이면 무시
	if (_level >= 30)
		return;

	_exp += amount;

	// 레벨업 체크 (S_GainExp보다 먼저 처리해야 정확한 exp 전송)
	while (_level < 30)
	{
		int32 requiredExp = GetMaxExp();
		if (requiredExp <= 0 || _exp < requiredExp)
			break;

		_exp -= requiredExp;
		ProcessLevelUp();
	}

	// S_GainExp 전송 (본인에게만, 레벨업 후 최종 exp)
	{
		Protocol::S_GainExp pkt;
		pkt.set_playerid(info.objectid());
		pkt.set_gainedexp(amount);
		pkt.set_currentexp(_exp);
		pkt.set_maxexp(GetMaxExp());

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_GainExp(pkt);
		if (session)
			session->Send(sendBuffer);
	}

	// PlayerExtra 갱신
	Protocol::PlayerExtra* extra = info.mutable_player();
	extra->set_exp(_exp);
	extra->set_maxexp(GetMaxExp());
}

void Player::ProcessLevelUp()
{
	_level++;

	const LevelData* data = GRoomDataManager.GetLevelData(_level);
	if (data)
		ApplyLevelStats(*data);

	// HP 전체 회복
	info.set_hp(info.maxhp());

	// PlayerExtra 갱신
	Protocol::PlayerExtra* extra = info.mutable_player();
	extra->set_level(_level);
	extra->set_exp(_exp);
	extra->set_maxexp(GetMaxExp());

	cout << "[Player] Level Up! " << info.name() << " -> Lv." << _level
		<< " (HP:" << info.maxhp() << " ATK:" << info.attack() << " DEF:" << info.defence() << ")" << endl;

	// S_LevelUp 브로드캐스트
	{
		Protocol::S_LevelUp pkt;
		pkt.set_playerid(info.objectid());
		pkt.set_newlevel(_level);
		pkt.set_maxhp(info.maxhp());
		pkt.set_attack(info.attack());
		pkt.set_defence(info.defence());
		pkt.set_maxexp(GetMaxExp());

		SendBufferRef sendBuffer = ServerPacketHandler::Make_S_LevelUp(pkt);
		if (room)
			room->Broadcast(sendBuffer);
	}
}

void Player::ApplyLevelStats(const LevelData& data)
{
	info.set_maxhp(data.maxHp);
	info.set_attack(data.attack);
	info.set_defence(data.defence);
}
