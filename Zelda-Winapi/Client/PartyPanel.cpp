#include "pch.h"
#include "PartyPanel.h"

#include "Sprite.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "MyPlayer.h"

PartyPanel::PartyPanel ( )
{
	SetDraggable ( true );
	SetDragHandleHeight ( 20 );
}

PartyPanel::~PartyPanel ( )
{
}

bool PartyPanel::PrepareLayout ( )
{
	Sprite* statusSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"PartyStatus" );
	if ( statusSprite == nullptr )
		return false;

	Vec2Int size = statusSprite->GetSize ( );
	SetSize ( size );

	Vec2 pos = GetPos ( );
	if ( ( int32 ) pos.x == 400 && ( int32 ) pos.y == 300 )
	{
		int32 left = 32;
		int32 top = 96;
		SetPos ( { ( float ) ( left + size.x / 2 ) , ( float ) ( top + size.y / 2 ) } );
	}

	return true;
}

void PartyPanel::Tick ( )
{
	if ( IsVisible ( ) == false )
		return;

	Super::Tick ( );
}

void PartyPanel::Render ( HDC hdc )
{
	if ( IsVisible ( ) == false )
		return;

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	if ( myPlayer->_partyMembers.empty ( ) )
		return;

	if ( PrepareLayout ( ) == false )
		return;

	Super::Render ( hdc );

	Sprite* statusSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"PartyStatus" );
	if ( statusSprite == nullptr )
		return;

	RECT rect = GetRect ( );
	int32 panelX = rect.left;
	int32 panelY = rect.top;
	int32 panelW = rect.right - rect.left;
	int32 panelH = rect.bottom - rect.top;

	::TransparentBlt ( hdc ,
		panelX , panelY ,
		panelW , panelH ,
		statusSprite->GetDC ( ) ,
		statusSprite->GetPos ( ).x , statusSprite->GetPos ( ).y ,
		panelW , panelH ,
		statusSprite->GetTransparent ( ) );

	SetBkMode ( hdc , TRANSPARENT );
	SetTextColor ( hdc , RGB ( 0 , 0 , 0 ) );

	TextOut ( hdc , panelX + 4 , panelY + 4 , L"Party" , 5 );

	const int32 rowH = 24;
	const int32 barW = 80;
	const int32 barH = 8;

	for ( int32 i = 0; i < ( int32 ) myPlayer->_partyMembers.size ( ); ++i )
	{
		const auto& member = myPlayer->_partyMembers[i];
		int32 y = panelY + 22 + i * rowH;

		wstring display = member.isLeader ? ( L"* " + member.name ) : ( L"  " + member.name );
		TextOut ( hdc , panelX + 4 , y , display.c_str ( ) , ( int32 ) display.length ( ) );

		int32 barX = panelX + 104;
		int32 barY = y + 2;

		HBRUSH darkBrush = CreateSolidBrush ( RGB ( 80 , 0 , 0 ) );
		RECT barBg = { barX , barY , barX + barW , barY + barH };
		FillRect ( hdc , &barBg , darkBrush );
		DeleteObject ( darkBrush );

		if ( member.maxHp > 0 && member.hp > 0 )
		{
			int32 fillW = barW * member.hp / member.maxHp;
			if ( fillW > 0 )
			{
				HBRUSH hpBrush = CreateSolidBrush ( RGB ( 0 , 200 , 0 ) );
				RECT hpRect = { barX , barY , barX + fillW , barY + barH };
				FillRect ( hdc , &hpRect , hpBrush );
				DeleteObject ( hpBrush );
			}
		}
	}
}

