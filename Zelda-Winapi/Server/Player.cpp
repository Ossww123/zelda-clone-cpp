#include "pch.h"
#include "Player.h"

Player::Player()
{
	info.set_name("PlayerName");
	info.set_hp(100);
	info.set_maxhp(100);
	info.set_attack(20);
	info.set_defence(5);
}

Player::~Player()
{
}

void Player::Update()
{
}
