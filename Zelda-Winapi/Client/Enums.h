#pragma once

enum class SceneType
{
	None,
	DevScene,
};

enum LAYER_TYPE
{
	LAYER_BACKGROUND,
	LAYER_OBJECT,
	LAYER_EFFECT,
	// ...
	LAYER_UI,

	LAYER_MAXCOUNT
};

#include "Enum.pb.h"
using Dir = Protocol::DIR_TYPE;

#define DIR_DOWN	Protocol::DIR_TYPE_DOWN
#define DIR_UP		Protocol::DIR_TYPE_UP
#define DIR_LEFT	Protocol::DIR_TYPE_LEFT
#define DIR_RIGHT	Protocol::DIR_TYPE_RIGHT

#define IDLE	Protocol::OBJECT_STATE_TYPE_IDLE
#define MOVE	Protocol::OBJECT_STATE_TYPE_MOVE
#define SKILL	Protocol::OBJECT_STATE_TYPE_SKILL

using ObjectState = Protocol::OBJECT_STATE_TYPE;

enum class WeaponType
{
	Sword,
	Bow,
	Staff,
};

enum EquipType
{
	EQUIP_TYPE_WEAPON = 0,
	EQUIP_TYPE_ARMOR  = 1,
	EQUIP_TYPE_POTION = 2,
};
