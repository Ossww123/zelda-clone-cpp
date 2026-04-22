#include "pch.h"
#include "UIManager.h"

#include "InputManager.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"
#include "MyPlayer.h"
#include "Sprite.h"
#include "InventoryPanel.h"
#include "PartyPanel.h"
#include "PartyInvitePanel.h"
#include "ChatPanel.h"
#include "RankingPanel.h"

void UIManager::Init ( HWND hWnd )
{
	_inventoryPanel = new InventoryPanel ( );
	_partyPanel = new PartyPanel ( );
	_partyInvitePanel = new PartyInvitePanel ( );
	_chatPanel = new ChatPanel ( );
	_rankingPanel = new RankingPanel ( );
	_chatPanel->Init ( hWnd );
}

void UIManager::Tick ( )
{
	_inputConsumed = false;

	// 채팅창이 열려 있으면 다른 입력 차단
	if ( _chatPanel && _chatPanel->IsVisible ( ) )
	{
		_chatPanel->Tick ( );

		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast<KeyType>( VK_ESCAPE ) ) )
		{
			_chatPanel->SetVisible ( false );
			GET_SINGLE ( InputManager )->SetInputLocked ( false );
			::SetWindowText ( _chatPanel->GetEditHandle ( ) , L"" );
		}

		_inputConsumed = true;
		return;
	}

	TickUIInput ( );
	TickPanels ( );
}

void UIManager::TickUIInput ( )
{
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::I ) )
	{
		if ( _inventoryPanel )
			_inventoryPanel->SetVisible ( !_inventoryPanel->IsVisible ( ) );
		GET_SINGLE ( SoundManager )->Play ( L"UISound" );
	}

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::R ) )
	{
		if ( _rankingPanel )
		{
			bool opening = !_rankingPanel->IsVisible ( );
			_rankingPanel->SetVisible ( opening );
			if ( opening )
			{
				SendBufferRef sendBuffer = ClientPacketHandler::Make_C_GetRanking ( );
				GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
			}
		}
	}

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast<KeyType>( VK_RETURN ) ) )
	{
		if ( _chatPanel && !_chatPanel->IsVisible ( ) )
		{
			_chatPanel->SetVisible ( true );
			GET_SINGLE ( InputManager )->SetInputLocked ( true );
			if ( _chatPanel->GetEditHandle ( ) )
				::SetFocus ( _chatPanel->GetEditHandle ( ) );
		}
	}

	// 파티 초대 수락/거절 (Y/N)
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer && myPlayer->_pendingInviteFrom != 0 )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::Y ) )
		{
			Protocol::C_PartyAnswer pkt;
			pkt.set_inviterid ( myPlayer->_pendingInviteFrom );
			pkt.set_accept ( true );
			GET_SINGLE ( NetworkManager )->SendPacket ( ClientPacketHandler::Make_C_PartyAnswer ( pkt ) );
			myPlayer->_pendingInviteFrom = 0;
			myPlayer->_pendingInviterName.clear ( );
			GET_SINGLE ( SoundManager )->Play ( L"UISound" );
			_inputConsumed = true;
			return;
		}
		else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::N ) )
		{
			Protocol::C_PartyAnswer pkt;
			pkt.set_inviterid ( myPlayer->_pendingInviteFrom );
			pkt.set_accept ( false );
			GET_SINGLE ( NetworkManager )->SendPacket ( ClientPacketHandler::Make_C_PartyAnswer ( pkt ) );
			myPlayer->_pendingInviteFrom = 0;
			myPlayer->_pendingInviterName.clear ( );
			GET_SINGLE ( SoundManager )->Play ( L"UISound" );
			_inputConsumed = true;
			return;
		}
	}

	// P키 파티 탈퇴
	if ( myPlayer && GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::P ) )
	{
		if ( !myPlayer->_partyMembers.empty ( ) )
			GET_SINGLE ( NetworkManager )->SendPacket ( ClientPacketHandler::Make_C_PartyLeave ( ) );
	}
}

void UIManager::TickPanels ( )
{
	if ( _inventoryPanel )
		_inventoryPanel->Tick ( );
	if ( _rankingPanel )
		_rankingPanel->Tick ( );
	if ( _partyPanel )
		_partyPanel->Tick ( );
	if ( _partyInvitePanel )
		_partyInvitePanel->Tick ( );
	if ( _chatPanel )
		_chatPanel->Tick ( );
}

void UIManager::Render ( HDC hdc )
{
	RenderHUD ( hdc );

	if ( _inventoryPanel )
		_inventoryPanel->Render ( hdc );
	if ( _rankingPanel )
		_rankingPanel->Render ( hdc );
	if ( _partyPanel )
		_partyPanel->Render ( hdc );
	if ( _partyInvitePanel )
		_partyInvitePanel->Render ( hdc );
	if ( _chatPanel )
		_chatPanel->Render ( hdc );
}

void UIManager::RenderHUD ( HDC hdc )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	const int32 baseX = 36;
	const int32 baseY = 36;

	Sprite* frame = GET_SINGLE ( ResourceManager )->GetSprite ( L"Status_Frame" );
	if ( frame )
	{
		::TransparentBlt ( hdc ,
			baseX , baseY ,
			frame->GetSize ( ).x , frame->GetSize ( ).y ,
			frame->GetDC ( ) ,
			frame->GetPos ( ).x , frame->GetPos ( ).y ,
			frame->GetSize ( ).x , frame->GetSize ( ).y ,
			frame->GetTransparent ( ) );
	}

	{
		Sprite* weaponSprite = nullptr;
		switch ( myPlayer->GetWeaponType ( ) )
		{
		case WeaponType::Sword: weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Sword_Icon" ); break;
		case WeaponType::Bow:   weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Bow_Icon" );   break;
		case WeaponType::Staff: weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Staff_Icon" ); break;
		}
		if ( weaponSprite )
		{
			::TransparentBlt ( hdc ,
				baseX , baseY ,
				weaponSprite->GetSize ( ).x , weaponSprite->GetSize ( ).y ,
				weaponSprite->GetDC ( ) ,
				weaponSprite->GetPos ( ).x , weaponSprite->GetPos ( ).y ,
				weaponSprite->GetSize ( ).x , weaponSprite->GetSize ( ).y ,
				weaponSprite->GetTransparent ( ) );
		}
	}

	{
		Sprite* hpBar = GET_SINGLE ( ResourceManager )->GetSprite ( L"Hp_Bar" );
		if ( hpBar )
		{
			int32 hp = myPlayer->info.hp ( );
			int32 maxHp = myPlayer->info.maxhp ( );
			int32 fullWidth = hpBar->GetSize ( ).x;
			int32 barWidth = ( maxHp > 0 ) ? ( fullWidth * hp / maxHp ) : 0;
			if ( barWidth > 0 )
			{
				::TransparentBlt ( hdc ,
					baseX + 99 , baseY + 9 ,
					barWidth , hpBar->GetSize ( ).y ,
					hpBar->GetDC ( ) ,
					hpBar->GetPos ( ).x , hpBar->GetPos ( ).y ,
					barWidth , hpBar->GetSize ( ).y ,
					hpBar->GetTransparent ( ) );
			}
		}
	}

	{
		Sprite* expBar = GET_SINGLE ( ResourceManager )->GetSprite ( L"Exp_Bar" );
		if ( expBar )
		{
			const Protocol::PlayerExtra& extra = myPlayer->info.player ( );
			int32 exp = extra.exp ( );
			int32 maxExp = extra.maxexp ( );
			int32 fullWidth = expBar->GetSize ( ).x;
			int32 barWidth = ( maxExp > 0 ) ? ( fullWidth * exp / maxExp ) : 0;
			if ( barWidth > 0 )
			{
				::TransparentBlt ( hdc ,
					baseX + 9 , baseY + 36 ,
					barWidth , expBar->GetSize ( ).y ,
					expBar->GetDC ( ) ,
					expBar->GetPos ( ).x , expBar->GetPos ( ).y ,
					barWidth , expBar->GetSize ( ).y ,
					expBar->GetTransparent ( ) );
			}
		}
	}
}

void UIManager::AddChatMessage ( const wstring& sender , const wstring& msg )
{
	if ( _chatPanel )
	{
		_chatPanel->SetVisible ( true );
		_chatPanel->AddMessage ( sender , msg );
	}
}

void UIManager::SetRankingData ( const vector<pair<wstring, int32>>& entries )
{
	if ( _rankingPanel )
		_rankingPanel->SetData ( entries );
}
