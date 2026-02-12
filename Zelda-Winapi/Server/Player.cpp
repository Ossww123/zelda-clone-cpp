#include "pch.h"
#include "Player.h"
#include "RoomDataManager.h"
#include "GameRoom.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"
#include "DBManager.h"

Player::Player()
{
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
	RecalcStats();
}

void Player::RecalcStats()
{
	const LevelData* data = GRoomDataManager.GetLevelData(_level);
	if (!data)
		return;

	int32 totalAttack = data->attack;
	int32 totalDefence = data->defence;

	if (_equipWeapon.itemId > 0)
	{
		const auto* item = GRoomDataManager.GetItemTemplate(_equipWeapon.itemId);
		if (item)
			totalAttack += item->value;
	}
	if (_equipArmor.itemId > 0)
	{
		const auto* item = GRoomDataManager.GetItemTemplate(_equipArmor.itemId);
		if (item)
			totalDefence += item->value;
	}

	info.set_attack(totalAttack);
	info.set_defence(totalDefence);
}

int32 Player::FindEmptySlot() const
{
	for (int32 i = 0; i < INVENTORY_SIZE; i++)
	{
		if (_storage[i].itemId == 0)
			return i;
	}
	return -1;
}

bool Player::AddItem(int32 itemId, int32 count)
{
	const ItemTemplateData* tmpl = GRoomDataManager.GetItemTemplate(itemId);
	if (!tmpl)
		return false;

	// 스택 가능한 아이템이면 기존 슬롯에 합산 시도
	if (tmpl->maxStack > 1)
	{
		for (int32 i = 0; i < INVENTORY_SIZE; i++)
		{
			if (_storage[i].itemId == itemId && _storage[i].count < tmpl->maxStack)
			{
				_storage[i].count = min(_storage[i].count + count, tmpl->maxStack);

				Protocol::S_AddItem pkt;
				pkt.set_itemid(itemId);
				pkt.set_slot(i);
				pkt.set_count(_storage[i].count);
				if (session)
					session->Send(ServerPacketHandler::Make_S_AddItem(pkt.itemid(), pkt.slot(), pkt.count()));

				cout << "[Player] " << info.name() << " acquired " << tmpl->name << " (slot " << i << ", count " << _storage[i].count << ")" << endl;
				return true;
			}
		}
	}

	// 빈 슬롯 찾아서 추가
	int32 slot = FindEmptySlot();
	if (slot < 0)
		return false;

	_storage[slot].itemId = itemId;
	_storage[slot].count = count;

	Protocol::S_AddItem pkt;
	pkt.set_itemid(itemId);
	pkt.set_slot(slot);
	pkt.set_count(count);
	if (session)
		session->Send(ServerPacketHandler::Make_S_AddItem(pkt.itemid(), pkt.slot(), pkt.count()));

	cout << "[Player] " << info.name() << " acquired " << tmpl->name << " (slot " << slot << ")" << endl;
	return true;
}

void Player::EquipItem(int32 slot)
{
	if (slot < 0 || slot >= INVENTORY_SIZE)
		return;
	if (_storage[slot].itemId == 0)
		return;

	const ItemTemplateData* tmpl = GRoomDataManager.GetItemTemplate(_storage[slot].itemId);
	if (!tmpl)
		return;

	InventorySlot* equipSlot = nullptr;
	int32 equipType = -1;

	if (tmpl->type == "weapon")
	{
		equipSlot = &_equipWeapon;
		equipType = 0;
	}
	else if (tmpl->type == "armor")
	{
		equipSlot = &_equipArmor;
		equipType = 1;
	}
	else if (tmpl->type == "consumable")
	{
		equipSlot = &_equipPotion;
		equipType = 2;
	}
	else
		return;

	// swap: 기존 장비와 교환
	InventorySlot old = *equipSlot;
	*equipSlot = _storage[slot];
	_storage[slot] = old;

	RecalcStats();

	// S_EquipItem 전송
	if (session)
	{
		session->Send(ServerPacketHandler::Make_S_EquipItem(
			equipType,
			slot,
			_storage[slot].itemId,
			_storage[slot].count,
			equipSlot->itemId,
			equipSlot->count,
			info.attack(),
			info.defence()
		));
	}
}

void Player::UnequipItem(int32 equipType)
{
	InventorySlot* equipSlot = nullptr;

	if (equipType == 0)
		equipSlot = &_equipWeapon;
	else if (equipType == 1)
		equipSlot = &_equipArmor;
	else if (equipType == 2)
		equipSlot = &_equipPotion;
	else
		return;

	if (equipSlot->itemId == 0)
		return;

	int32 slot = FindEmptySlot();
	if (slot < 0)
		return; // 인벤 풀

	_storage[slot] = *equipSlot;
	equipSlot->itemId = 0;
	equipSlot->count = 0;

	RecalcStats();

	if (session)
	{
		session->Send(ServerPacketHandler::Make_S_UnequipItem(
			equipType,
			slot,
			info.attack(),
			info.defence()
		));
	}
}

void Player::UseItem(int32 slot)
{
	InventorySlot* targetSlot = nullptr;
	int32 equipType = -1;

	// 장착 슬롯(포션)에서 사용
	if (slot == -1)
	{
		targetSlot = &_equipPotion;
		equipType = 2;
	}
	else if (slot >= 0 && slot < INVENTORY_SIZE)
	{
		targetSlot = &_storage[slot];
		equipType = -1;
	}
	else
		return;

	if (targetSlot->itemId == 0)
		return;

	const ItemTemplateData* tmpl = GRoomDataManager.GetItemTemplate(targetSlot->itemId);
	if (!tmpl || tmpl->type != "consumable")
		return;

	// HP 회복
	int32 newHp = min(info.hp() + tmpl->value, info.maxhp());
	info.set_hp(newHp);

	targetSlot->count--;
	if (targetSlot->count <= 0)
	{
		targetSlot->itemId = 0;
		targetSlot->count = 0;
	}

	if (session)
	{
		session->Send(ServerPacketHandler::Make_S_UseItem(
			equipType,
			targetSlot->count,
			newHp
		));
	}
}

void Player::SendInventoryData()
{
	if (!session)
		return;

	Protocol::S_InventoryData pkt;

	for (int32 i = 0; i < INVENTORY_SIZE; i++)
	{
		if (_storage[i].itemId == 0)
			continue;
		Protocol::ItemInfo* item = pkt.add_items();
		item->set_itemid(_storage[i].itemId);
		item->set_count(_storage[i].count);
		item->set_slot(i);
	}

	{
		Protocol::ItemInfo* w = pkt.mutable_equippedweapon();
		w->set_itemid(_equipWeapon.itemId);
		w->set_count(_equipWeapon.count);
	}
	{
		Protocol::ItemInfo* a = pkt.mutable_equippedarmor();
		a->set_itemid(_equipArmor.itemId);
		a->set_count(_equipArmor.count);
	}
	{
		Protocol::ItemInfo* p = pkt.mutable_equippedpotion();
		p->set_itemid(_equipPotion.itemId);
		p->set_count(_equipPotion.count);
	}

	session->Send(ServerPacketHandler::MakeSendBuffer(pkt, S_InventoryData));
}

void Player::ApplyFromSaveData(const PlayerSaveData& data)
{
	info.set_name(data.name);
	_level = data.level;
	_exp = data.exp;

	// 레벨 기반 스탯 적용
	const LevelData* levelData = GRoomDataManager.GetLevelData(_level);
	if (levelData)
	{
		info.set_maxhp(levelData->maxHp);
		info.set_attack(levelData->attack);
		info.set_defence(levelData->defence);
	}

	// HP 복원 (maxHp 초과 방지)
	info.set_hp(min(data.hp, info.maxhp()));

	// 인벤토리 복원
	for (int32 i = 0; i < INVENTORY_SIZE; i++)
		_storage[i] = data.storage[i];

	_equipWeapon = data.equipWeapon;
	_equipArmor = data.equipArmor;
	_equipPotion = data.equipPotion;

	// 장비 보정 반영
	RecalcStats();

	// PlayerExtra 갱신
	Protocol::PlayerExtra* extra = info.mutable_player();
	extra->set_level(_level);
	extra->set_exp(_exp);
	extra->set_maxexp(GetMaxExp());

	cout << "[Player] Applied save data: " << data.name << " Lv." << _level << endl;
}

PlayerSaveData Player::ToSaveData() const
{
	PlayerSaveData data;
	data.name = info.name();
	data.level = _level;
	data.exp = _exp;
	data.hp = info.hp();

	for (int32 i = 0; i < INVENTORY_SIZE; i++)
		data.storage[i] = _storage[i];

	data.equipWeapon = _equipWeapon;
	data.equipArmor = _equipArmor;
	data.equipPotion = _equipPotion;

	return data;
}
