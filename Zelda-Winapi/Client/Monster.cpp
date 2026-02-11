#include "pch.h"
#include "Monster.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "TimeManager.h"

#include "Flipbook.h"
#include "Sprite.h"
#include "SceneManager.h"
#include "DevScene.h"
#include "Player.h"
#include "HitEffect.h"

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

	SetState(MOVE);
	SetState(IDLE);
}

void Monster::Tick()
{
	Super::Tick();

	// TODO
}

void Monster::Render(HDC hdc)
{
	Super::Render(hdc);

	// Enemy HP Bar
	Sprite* hpFrame = GET_SINGLE ( ResourceManager )->GetSprite ( L"Enemy_Hp_Frame" );
	Sprite* hpBar = GET_SINGLE ( ResourceManager )->GetSprite ( L"Enemy_Hp_Bar" );
	if ( hpFrame == nullptr || hpBar == nullptr )
		return;

	Vec2 cameraPos = GET_SINGLE ( SceneManager )->GetCameraPos ( );
	int32 cameraOffsetX = ( int32 ) cameraPos.x - GWinSizeX / 2;
	int32 cameraOffsetY = ( int32 ) cameraPos.y - GWinSizeY / 2;

	int32 screenX = ( int32 ) _pos.x - cameraOffsetX;
	int32 screenY = ( int32 ) _pos.y - cameraOffsetY;

	// Frame 위치: 몬스터 스프라이트(100x100) 위에 배치
	int32 frameW = hpFrame->GetSize ( ).x;  // 102
	int32 frameH = hpFrame->GetSize ( ).y;  // 12
	int32 frameX = screenX - frameW / 2;
	int32 frameY = screenY - 50 - frameH - 2;

	::TransparentBlt ( hdc ,
		frameX , frameY ,
		frameW , frameH ,
		hpFrame->GetDC ( ) ,
		hpFrame->GetPos ( ).x , hpFrame->GetPos ( ).y ,
		frameW , frameH ,
		hpFrame->GetTransparent ( ) );

	// HP Bar: Frame 기준 (3, 3) 오프셋
	int32 hp = info.hp ( );
	int32 maxHp = info.maxhp ( );
	int32 fullBarWidth = hpBar->GetSize ( ).x;  // 96
	int32 barWidth = ( maxHp > 0 ) ? ( fullBarWidth * hp / maxHp ) : 0;

	if ( barWidth > 0 )
	{
		::TransparentBlt ( hdc ,
			frameX + 3 , frameY + 3 ,
			barWidth , hpBar->GetSize ( ).y ,
			hpBar->GetDC ( ) ,
			hpBar->GetPos ( ).x , hpBar->GetPos ( ).y ,
			barWidth , hpBar->GetSize ( ).y ,
			hpBar->GetTransparent ( ) );
	}
}

void Monster::TickIdle()
{
}

void Monster::TickMove()
{
	float deltaTime = GET_SINGLE(TimeManager)->GetDeltaTime();

	Vec2 dir = (_destPos - _pos);
	if (dir.Length() < 5.f)
	{
		SetState(IDLE);
		_pos = _destPos;
	}
	else
	{
		bool horizontal = abs(dir.x) > abs(dir.y);
		if (horizontal)
			SetDir(dir.x < 0 ? DIR_LEFT : DIR_RIGHT);
		else
			SetDir(dir.y < 0 ? DIR_UP : DIR_DOWN);

		switch (info.dir())
		{
		case DIR_UP:
			_pos.y -= 50 * deltaTime;
			break;
		case DIR_DOWN:
			_pos.y += 50 * deltaTime;
			break;
		case DIR_LEFT:
			_pos.x -= 50 * deltaTime;
			break;
		case DIR_RIGHT:
			_pos.x += 50 * deltaTime;
			break;
		}
	}
}

void Monster::TickSkill()
{
	if (_flipbook == nullptr)
		return;

	if ( IsAnimationEnded ( ) )
		SetState ( IDLE );
}

void Monster::UpdateAnimation()
{
	SetFlipbook(_flipbookMove[info.dir() ] );
}
