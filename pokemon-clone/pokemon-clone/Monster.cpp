#include "pch.h"
#include "Monster.h"

#include "InputManager.h"

Monster::Monster ( ) : Object ( ObjectType::Monster )
{
}

Monster::~Monster ( )
{
}

void Monster::Init ( )
{
	_stat.hp = 100;
	_stat.maxHp = 100;
	_stat.speed = 10;
}

void Monster::Update ( )
{
	Vector mousePos = GET_SINGLE ( InputManager )->GetMousePos ( );

	// start -> end
	Vector v1 = _end - _start;

	// start -> mousePos
	Vector v2 = mousePos - _start;

	float maxLength = v1.Length ( );
	v1.Normalize ( );

	float dot = v1.Dot ( v2 );
	Pos pos = _start + v1 * dot;

	if ( dot < 0 || dot > maxLength )
		return;

	_pos = pos;
}

void Monster::Render ( HDC hdc )
{
	Utils::DrawCircle ( hdc , _pos , 50 );

	HPEN pen = ::CreatePen ( PS_SOLID , 1 , RGB ( 255 , 0 , 0 ) );
	HPEN oldPen = ( HPEN )::SelectObject ( hdc , pen );
	{
		Utils::DrawLine ( hdc , _start , _end );
	}
	::SelectObject ( hdc , oldPen );
	::DeleteObject ( pen );
}
