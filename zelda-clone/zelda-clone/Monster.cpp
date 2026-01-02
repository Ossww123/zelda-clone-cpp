#include "pch.h"
#include "Monster.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "SceneManager.h"
#include "DevScene.h"

Monster::Monster()
{
	_flipbookMove[DIR_UP] = GET_SINGLE(ResourceManager)->GetFlipbook(L"FB_SnakeUp");
	_flipbookMove[DIR_DOWN] = GET_SINGLE(ResourceManager)->GetFlipbook(L"FB_SnakeDown");
	_flipbookMove[DIR_LEFT] = GET_SINGLE(ResourceManager)->GetFlipbook(L"FB_SnakeLeft");
	_flipbookMove[DIR_RIGHT] = GET_SINGLE(ResourceManager)->GetFlipbook(L"FB_SnakeRight");
}

Monster::~Monster()
{
}

void Monster::BeginPlay()
{
	Super::BeginPlay();

	SetState(ObjectState::Move);
	SetState(ObjectState::Idle);
}

void Monster::Tick()
{
	Super::Tick();

	// TODO
}

void Monster::Render(HDC hdc)
{
	Super::Render(hdc);

	// TODO
}

void Monster::TickIdle()
{
}

void Monster::TickMove()
{
}

void Monster::TickSkill()
{
}

void Monster::UpdateAnimation()
{
	SetFlipbook(_flipbookMove[_dir]);
}
