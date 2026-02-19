#include "pch.h"
#include "PartyInvitePanel.h"

#include "Sprite.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "MyPlayer.h"

PartyInvitePanel::PartyInvitePanel ( )
{
}

PartyInvitePanel::~PartyInvitePanel ( )
{
}

bool PartyInvitePanel::PrepareLayout ( int32& outW , int32& outH ) const
{
	Sprite* inviteSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"PartyInvite" );
	outW = 300;
	outH = 80;
	if ( inviteSprite )
	{
		outW = inviteSprite->GetSize ( ).x;
		outH = inviteSprite->GetSize ( ).y;
	}

	return inviteSprite != nullptr;
}

void PartyInvitePanel::Tick ( )
{
	Super::Tick ( );
}

void PartyInvitePanel::Render ( HDC hdc )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	if ( myPlayer->_pendingInviteFrom == 0 )
		return;

	int32 popW = 0;
	int32 popH = 0;
	if ( PrepareLayout ( popW , popH ) == false )
		return;

	Sprite* inviteSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"PartyInvite" );
	if ( inviteSprite == nullptr )
		return;

	int32 popX = ( GWinSizeX - popW ) / 2;
	int32 popY = ( GWinSizeY - popH ) / 2 - 50;

	SetSize ( { popW , popH } );
	SetPos ( { ( float ) ( popX + popW / 2 ) , ( float ) ( popY + popH / 2 ) } );

	Super::Render ( hdc );

	::TransparentBlt ( hdc ,
		popX , popY ,
		popW , popH ,
		inviteSprite->GetDC ( ) ,
		inviteSprite->GetPos ( ).x , inviteSprite->GetPos ( ).y ,
		popW , popH ,
		inviteSprite->GetTransparent ( ) );

	SetBkMode ( hdc , TRANSPARENT );
	SetTextColor ( hdc , RGB ( 50 , 50 , 50 ) );

	wstring msg = myPlayer->_pendingInviterName + L" invited you to party";
	RECT textRect = { popX + 10 , popY + 15 , popX + popW - 10 , popY + 40 };
	DrawText ( hdc , msg.c_str ( ) , ( int32 ) msg.length ( ) , &textRect , DT_CENTER );

	wstring hint = L"[Y] Accept    [N] Decline";
	RECT hintRect = { popX + 10 , popY + 45 , popX + popW - 10 , popY + 70 };
	SetTextColor ( hdc , RGB ( 0 , 0 , 0 ) );
	DrawText ( hdc , hint.c_str ( ) , ( int32 ) hint.length ( ) , &hintRect , DT_CENTER );
}

