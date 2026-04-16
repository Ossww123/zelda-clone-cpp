#include "pch.h"
#include "RankingPanel.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "Sprite.h"

static const int32 PANEL_W = 220;
static const int32 PANEL_H = 260;
static const int32 ROW_H   = 22;

RankingPanel::RankingPanel ( )
{
	SetVisible ( false );
	SetDraggable ( true );
	SetDragHandleHeight ( 30 );
	SetSize ( { PANEL_W, PANEL_H } );
	SetPos ( { (float)( GWinSizeX - PANEL_W / 2 - 10 ), (float)( GWinSizeY / 2 ) + 40.f } );
}

RankingPanel::~RankingPanel ( )
{
}

void RankingPanel::SetData ( const vector<pair<wstring, int32>>& entries )
{
	_entries = entries;
}

void RankingPanel::Tick ( )
{
	if ( IsVisible ( ) == false )
		return;

	Super::Tick ( );
}

void RankingPanel::Render ( HDC hdc )
{
	if ( IsVisible ( ) == false )
		return;

	RECT rect = GetRect ( );
	int32 x = rect.left;
	int32 y = rect.top;

	// 배경
	Sprite* bg = GET_SINGLE ( ResourceManager )->GetSprite ( L"RankingPanel" );
	if ( bg )
	{
		::TransparentBlt ( hdc,
			x, y,
			PANEL_W, PANEL_H,
			bg->GetDC ( ),
			bg->GetPos ( ).x, bg->GetPos ( ).y,
			bg->GetSize ( ).x, bg->GetSize ( ).y,
			bg->GetTransparent ( ) );
	}
	else
	{
		HBRUSH bgBrush = ::CreateSolidBrush ( RGB ( 20, 20, 40 ) );
		HBRUSH oldBrush = ( HBRUSH ) ::SelectObject ( hdc, bgBrush );
		HPEN bgPen = ::CreatePen ( PS_SOLID, 2, RGB ( 180, 160, 80 ) );
		HPEN oldPen = ( HPEN ) ::SelectObject ( hdc, bgPen );
		::Rectangle ( hdc, x, y, x + PANEL_W, y + PANEL_H );
		::SelectObject ( hdc, oldBrush );
		::SelectObject ( hdc, oldPen );
		::DeleteObject ( bgBrush );
		::DeleteObject ( bgPen );
	}

	// 타이틀
	::SetBkMode ( hdc, TRANSPARENT );
	::SetTextColor ( hdc, RGB ( 255, 220, 80 ) );
	RECT titleRect = { x, y + 6, x + PANEL_W, y + 30 };
	::DrawText ( hdc, L"  레벨 랭킹 TOP 10", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

	// 항목
	::SetTextColor ( hdc, RGB ( 220, 220, 220 ) );
	for ( int32 i = 0; i < ( int32 ) _entries.size ( ); i++ )
	{
		wstring line = to_wstring ( i + 1 ) + L".  " + _entries[ i ].first
			+ L"  Lv." + to_wstring ( _entries[ i ].second );
		RECT rowRect = { x + 12, y + 36 + i * ROW_H, x + PANEL_W - 8, y + 36 + ( i + 1 ) * ROW_H };
		::DrawText ( hdc, line.c_str ( ), -1, &rowRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}

	if ( _entries.empty ( ) )
	{
		::SetTextColor ( hdc, RGB ( 140, 140, 140 ) );
		RECT emptyRect = { x, y + 36, x + PANEL_W, y + 36 + ROW_H };
		::DrawText ( hdc, L"  데이터 없음", -1, &emptyRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}
}
