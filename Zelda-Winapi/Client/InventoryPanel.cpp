#include "pch.h"
#include "InventoryPanel.h"

#include "Sprite.h"
#include "ResourceManager.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"

InventoryPanel::InventoryPanel ( )
{
	SetVisible ( false );
	SetDraggable ( true );
	SetDragHandleHeight ( 30 );
}

InventoryPanel::~InventoryPanel ( )
{
}

bool InventoryPanel::PrepareLayout ( )
{
	Sprite* invSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Inventory" );
	if ( invSprite == nullptr )
		return false;

	Vec2Int size = invSprite->GetSize ( );
	SetSize ( size );

	Vec2 pos = GetPos ( );
	if ( ( int32 ) pos.x == 400 && ( int32 ) pos.y == 300 )
	{
		SetPos ( { ( float ) ( GWinSizeX / 2 ), ( float ) ( GWinSizeY / 2 ) } );
	}

	return true;
}

void InventoryPanel::Tick ( )
{
	if ( IsVisible ( ) == false )
		return;

	if ( PrepareLayout ( ) == false )
		return;

	Super::Tick ( );
	HandleInventoryClick ( );
}

void InventoryPanel::Render ( HDC hdc )
{
	if ( IsVisible ( ) == false )
		return;

	if ( PrepareLayout ( ) == false )
		return;

	Super::Render ( hdc );

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	Sprite* invSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Inventory" );
	if ( invSprite == nullptr )
		return;

	RECT rect = GetRect ( );
	int32 invX = rect.left;
	int32 invY = rect.top;
	int32 invW = rect.right - rect.left;
	int32 invH = rect.bottom - rect.top;

	::TransparentBlt ( hdc ,
		invX , invY ,
		invW , invH ,
		invSprite->GetDC ( ) ,
		invSprite->GetPos ( ).x , invSprite->GetPos ( ).y ,
		invW , invH ,
		invSprite->GetTransparent ( ) );

	{
		const Protocol::PlayerExtra& extra = myPlayer->info.player ( );

		int32 level = extra.level ( );
		int32 hp = myPlayer->info.hp ( );
		int32 maxHp = myPlayer->info.maxhp ( );
		int32 attack = myPlayer->info.attack ( );
		int32 defence = myPlayer->info.defence ( );

		wchar_t buf[ 256 ];
		swprintf_s ( buf ,
			L"Level: %d\nMax HP: %d\nHP: %d\nATK: %d\nDEF: %d" ,
			level , maxHp , hp , attack , defence );

		::SetBkMode ( hdc , TRANSPARENT );
		::SetTextColor ( hdc , RGB ( 255 , 255 , 255 ) );

		static HFONT sFont = nullptr;
		if ( sFont == nullptr )
		{
			sFont = ::CreateFontW (
				18 , 0 , 0 , 0 , FW_BOLD , FALSE , FALSE , FALSE ,
				DEFAULT_CHARSET , OUT_DEFAULT_PRECIS , CLIP_DEFAULT_PRECIS ,
				DEFAULT_QUALITY , DEFAULT_PITCH | FF_DONTCARE ,
				L"Consolas" );
		}

		HGDIOBJ oldFont = nullptr;
		if ( sFont ) oldFont = ::SelectObject ( hdc , sFont );

		RECT rc;
		rc.left = invX + 184;
		rc.top = invY + 32;
		rc.right = invX + invW - 16;
		rc.bottom = invY + 150;

		::DrawTextW ( hdc , buf , -1 , &rc , DT_LEFT | DT_TOP | DT_WORDBREAK );

		if ( oldFont ) ::SelectObject ( hdc , oldFont );
	}

	auto renderItem = [&] ( int32 itemId , int32 count , int32 relX , int32 relY )
	{
		if ( itemId == 0 )
			return;
		Sprite* sp = GetItemSprite ( itemId );
		if ( sp == nullptr )
			return;
		::TransparentBlt ( hdc ,
			invX + relX , invY + relY ,
			32 , 32 ,
			sp->GetDC ( ) ,
			sp->GetPos ( ).x , sp->GetPos ( ).y ,
			sp->GetSize ( ).x , sp->GetSize ( ).y ,
			sp->GetTransparent ( ) );

		if ( count > 1 )
		{
			wchar_t buf[ 8 ];
			swprintf_s ( buf , L"%d" , count );
			::SetBkMode ( hdc , TRANSPARENT );
			::SetTextColor ( hdc , RGB ( 255 , 255 , 255 ) );
			::TextOut ( hdc , invX + relX + 18 , invY + relY + 18 , buf , ( int ) wcslen ( buf ) );
		}
	};

	renderItem ( myPlayer->_equipWeapon.itemId , myPlayer->_equipWeapon.count , 98 , 38 );
	renderItem ( myPlayer->_equipArmor.itemId , myPlayer->_equipArmor.count , 98 , 80 );
	renderItem ( myPlayer->_equipPotion.itemId , myPlayer->_equipPotion.count , 290 , 68 );

	for ( int32 i = 0; i < MyPlayer::INVENTORY_SIZE; i++ )
	{
		int32 col = i % 9;
		int32 row = i / 9;
		int32 slotX = 16 + col * 36;
		int32 slotY = 168 + row * 36;

		renderItem ( myPlayer->_storage[ i ].itemId , myPlayer->_storage[ i ].count , slotX , slotY );
	}
}

void InventoryPanel::HandleInventoryClick ( )
{
	if ( !GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::RightMouse ) )
		return;

	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	Sprite* invSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Inventory" );
	if ( invSprite == nullptr )
		return;

	RECT rect = GetRect ( );
	int32 invX = rect.left;
	int32 invY = rect.top;
	int32 invW = rect.right - rect.left;
	int32 invH = rect.bottom - rect.top;

	POINT mouse = GET_SINGLE ( InputManager )->GetMousePos ( );
	int32 mx = mouse.x - invX;
	int32 my = mouse.y - invY;

	if ( mx < 0 || my < 0 || mx >= invW || my >= invH )
		return;

	if ( mx >= 98 && mx < 130 && my >= 38 && my < 70 )
	{
		if ( myPlayer->_equipWeapon.itemId > 0 )
		{
			Protocol::C_UnequipItem unequipPkt;
			unequipPkt.set_equiptype ( 0 );
			SendBufferRef sb = ClientPacketHandler::Make_C_UnequipItem ( unequipPkt );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
		}
		return;
	}
	if ( mx >= 98 && mx < 130 && my >= 80 && my < 112 )
	{
		if ( myPlayer->_equipArmor.itemId > 0 )
		{
			Protocol::C_UnequipItem unequipPkt;
			unequipPkt.set_equiptype ( 1 );
			SendBufferRef sb = ClientPacketHandler::Make_C_UnequipItem ( unequipPkt );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
		}
		return;
	}
	if ( mx >= 290 && mx < 322 && my >= 68 && my < 100 )
	{
		if ( myPlayer->_equipPotion.itemId > 0 )
		{
			Protocol::C_UseItem useItemPkt;
			useItemPkt.set_slot ( -1 );
			SendBufferRef sb = ClientPacketHandler::Make_C_UseItem ( useItemPkt );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
		}
		return;
	}

	for ( int32 i = 0; i < MyPlayer::INVENTORY_SIZE; i++ )
	{
		int32 col = i % 9;
		int32 row = i / 9;
		int32 slotX = 16 + col * 36;
		int32 slotY = 168 + row * 36;

		if ( mx >= slotX && mx < slotX + 32 && my >= slotY && my < slotY + 32 )
		{
			if ( myPlayer->_storage[ i ].itemId > 0 )
			{
				Protocol::C_EquipItem equipPkt;
				equipPkt.set_slot ( i );
				SendBufferRef sb = ClientPacketHandler::Make_C_EquipItem ( equipPkt );
				GET_SINGLE ( NetworkManager )->SendPacket ( sb );
			}
			return;
		}
	}
}

Sprite* InventoryPanel::GetItemSprite ( int32 itemId )
{
	switch ( itemId )
	{
	case 2001: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Potion_A" );
	case 3001: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Sword_A" );
	case 3002: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Sword_B" );
	case 3003: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Sword_C" );
	case 4001: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Armor_A" );
	case 4002: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Armor_B" );
	case 4003: return GET_SINGLE ( ResourceManager )->GetSprite ( L"Armor_C" );
	default: return nullptr;
	}
}

