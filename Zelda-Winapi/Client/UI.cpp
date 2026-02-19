#include "pch.h"
#include "UI.h"

#include "InputManager.h"

UI::UI ( )
{
}

UI::~UI ( )
{
}

void UI::BeginPlay ( )
{
}

void UI::Tick ( )
{
	if ( _visible == false || _enabled == false )
		return;
}

void UI::Render ( HDC hdc )
{
	if ( _visible == false )
		return;
}

RECT UI::GetRect ( )
{
	RECT rect =
	{
		_pos.x - _size.x / 2,
		_pos.y - _size.y / 2,
		_pos.x + _size.x / 2,
		_pos.y + _size.y / 2,
	};

	return rect;
}

bool UI::IsMouseInRect ( )
{
	if ( _visible == false )
		return false;

	RECT rect = GetRect ( );

	POINT mousePos = GET_SINGLE ( InputManager )->GetMousePos ( );

	return ::PtInRect ( &rect , mousePos );
}
