#include "pch.h"
#include "DevScene.h"

#include "Client.h"

#include "Utils.h"
#include "Texture.h"
#include "Sprite.h"
#include "Actor.h"
#include "SpriteActor.h"
#include "Flipbook.h"
#include "Player.h"
#include "Arrow.h"
#include "UI.h"
#include "Button.h"
#include "Tilemap.h"
#include "TilemapActor.h"
#include "Sound.h"
#include "Monster.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"
#include "InventoryPanel.h"
#include "PartyPanel.h"
#include "PartyInvitePanel.h"
#include "ChatPanel.h"
#include "DevSceneResourceLoader.h"

DevScene::DevScene ( )
{
}

DevScene::~DevScene ( )
{
	SAFE_DELETE ( _inventoryPanel );
	SAFE_DELETE ( _partyPanel );
	SAFE_DELETE ( _partyInvitePanel );
	SAFE_DELETE ( _chatPanel );
}

void DevScene::Init ( )
{
	LoadSceneTextures ( );
	CreateSceneSprites ( );
	LoadMap ( );
	LoadPlayer ( );
	LoadMonster ( );
	LoadProjectiles ( );
	LoadEffect ( );
	LoadTilemap ( );

	_currentMapId = Protocol::MAP_ID_TOWN;
	_hasMapId = true;

	LoadSceneSounds ( );

	_inventoryPanel = new InventoryPanel ( );
	_partyPanel = new PartyPanel ( );
	_partyInvitePanel = new PartyInvitePanel ( );
	_chatPanel = new ChatPanel ( );
	_chatPanel->Init ( g_hWnd);

	CreateMapButtons ( );
	Super::Init ( );
}

void DevScene::Update ( )
{
	if ( !_loggedIn )
	{
		UpdateLogin ( );
		return;
	}

	Super::Update ( );

	// 채팅창 열려있으면 게임 로직 입력 처리 건너뜀
	if ( _chatPanel && _chatPanel->IsVisible ( ) )
	{
		_chatPanel->Tick ( );

		// 채팅창 닫기
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_ESCAPE ) ) )
		{
			_chatPanel->SetVisible ( false );
			GET_SINGLE ( InputManager )->SetInputLocked ( false );
			::SetWindowText ( _chatPanel->GetEditHandle ( ) , L"" );
		}
		return;
	}

	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::I ) )
	{
		if ( _inventoryPanel )
			_inventoryPanel->SetVisible ( !_inventoryPanel->IsVisible ( ) );
		GET_SINGLE ( SoundManager )->Play ( L"UISound" );
	}
	if ( _inventoryPanel )
		_inventoryPanel->Tick ( );
	if ( _partyPanel )
		_partyPanel->Tick ( );
	if ( _partyInvitePanel )
		_partyInvitePanel->Tick ( );
	if ( _chatPanel )
	{
		// Enter 키로 채팅창 토글
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_RETURN ) ) )
		{
			if ( _chatPanel->IsVisible ( ) == false )
			{
				_chatPanel->SetVisible ( true );
				GET_SINGLE ( InputManager )->SetInputLocked ( true );
				if ( _chatPanel->GetEditHandle ( ) )
					::SetFocus ( _chatPanel->GetEditHandle ( ) );
			}
		}
		_chatPanel->Tick ( );
	}
	HandlePartyInput ( );
}

void DevScene::Render ( HDC hdc )
{
	if ( !_loggedIn )
	{
		RenderLogin ( hdc );
		return;
	}

	Super::Render ( hdc );
	RenderHUD ( hdc );

	if ( _inventoryPanel )
		_inventoryPanel->Render ( hdc );
	if ( _partyPanel )
		_partyPanel->Render ( hdc );
	if ( _partyInvitePanel )
		_partyInvitePanel->Render ( hdc );
	if ( _chatPanel )
		_chatPanel->Render ( hdc );
}

void DevScene::AddActor ( Actor* actor )
{
	Super::AddActor ( actor );
}

void DevScene::RemoveActor ( Actor* actor )
{
	Super::RemoveActor ( actor );
}

void DevScene::ChangeMap ( Protocol::MAP_ID mapId )
{
	_currentMapId = mapId;
	_hasMapId = true;

	ClearWorldActors ( );
	ChangeBackground ( mapId );

	// 타일맵 교체
	if ( mapId == Protocol::MAP_ID_TOWN )
	{
		LoadTilemap ( L"Tilemap\\Tilemap_01.txt" );
	}
	else if ( mapId == Protocol::MAP_ID_DUNGEON )
	{
		LoadTilemap ( L"Tilemap\\Tilemap_02.txt" );
	}
}

void DevScene::ChangeBackground ( Protocol::MAP_ID mapId )
{
	if ( _background == nullptr )
		return;

	Sprite* sprite = nullptr;

	if ( mapId == Protocol::MAP_ID_TOWN )
		sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage01" );
	else if ( mapId == Protocol::MAP_ID_DUNGEON )
		sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage02" );

	if ( sprite == nullptr )
		return;

	_background->SetSprite ( sprite );

	const Vec2Int size = sprite->GetSize ( );
	_background->SetPos ( Vec2 ( size.x / 2 , size.y / 2 ) );
}

void DevScene::Handle_S_AddObject ( Protocol::S_AddObject& pkt )
{
	uint64 myPlayerId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );

	const int32 size = pkt.objects_size ( );
	for ( int32 i = 0; i < size; i++ )
	{
		const Protocol::ObjectInfo& info = pkt.objects ( i );
		if ( myPlayerId == info.objectid ( ) )
			continue;

		if ( info.objecttype ( ) == Protocol::OBJECT_TYPE_PLAYER )
		{
			Player* player = SpawnObject<Player> ( Vec2Int{ info.posx ( ), info.posy ( ) } );
			player->SetDir ( info.dir ( ) );
			player->SetState ( info.state ( ) );
			player->info = info;
		}
		else if ( info.objecttype ( ) == Protocol::OBJECT_TYPE_MONSTER )
		{
			Monster* monster = SpawnObject<Monster> ( Vec2Int{ info.posx ( ), info.posy ( ) } );
			monster->info = info;
			monster->SetDir ( info.dir ( ) );
			monster->SetState ( info.state ( ) );
			monster->SetCellPos ( Vec2Int{ info.posx ( ), info.posy ( ) } , true );
		}
		else if ( info.objecttype() == Protocol::OBJECT_TYPE_PROJECTILE)
		{
			 Arrow* arrow = SpawnObject<Arrow>( Vec2Int{info.posx(), info.posy()});
			 arrow->info = info;
			 arrow->SetDir(info.dir());
			 arrow->SetState(info.state());
			 arrow->SetCellPos({info.posx(), info.posy()}, true);
		}
	}
}

void DevScene::Handle_S_RemoveObject ( Protocol::S_RemoveObject& pkt )
{
	const int32 size = pkt.ids_size ( );
	for ( int32 i = 0; i < size; i++ )
	{
		int32 id = pkt.ids ( i );

		GameObject* object =  GetObject( id );
		if ( object )
			RemoveActor ( object );
	}
}

GameObject* DevScene::GetObject ( uint64 id )
{
	for ( Actor* actor : _actors[ LAYER_OBJECT ] )
	{
		GameObject* gameObject = dynamic_cast< GameObject* >( actor );
		if ( gameObject && gameObject->info.objectid ( ) == id )
			return gameObject;
	}

	return nullptr;
}

Player* DevScene::FindClosestPlayer ( Vec2Int cellPos )
{
	float best = FLT_MAX;
	Player* ret = nullptr;

	for ( Actor* actor : _actors[ LAYER_OBJECT ] )
	{
		Player* player = dynamic_cast< Player* >( actor );
		if ( player )
		{
			Vec2Int dir = cellPos - player->GetCellPos ( );
			float dist = dir.LengthSquared ( );
			if ( dist < best )
			{
				dist = best;
				ret = player;
			}
		}
	}
	return ret;
}

bool DevScene::CanGo ( Vec2Int cellPos )
{
	if ( _tilemapActor == nullptr )
		return false;

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return false;

	Tile* tile = tm->GetTileAt ( cellPos );
	if ( tile == nullptr )
		return false;

	if ( GetCreatureAt ( cellPos ) != nullptr )
		return false;

	return tile->value != 1;
}

Vec2 DevScene::ConvertPos ( Vec2Int cellPos )
{
	Vec2 ret = {};

	if ( _tilemapActor == nullptr )
		return ret;

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return ret;

	int32 size = tm->GetTileSize ( );
	Vec2 pos = _tilemapActor->GetPos ( );

	ret.x = pos.x + cellPos.x * size + ( size / 2 );
	ret.y = pos.y + cellPos.y * size + ( size / 2 );

	return ret;
}

Vec2Int DevScene::GetWorldPixelSize ( ) const
{
	if ( _tilemapActor == nullptr )
		return { 0, 0 };

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return { 0, 0 };

	Vec2Int mapSize = tm->GetMapSize ( );
	int32 tileSize = tm->GetTileSize ( );

	return { mapSize.x * tileSize, mapSize.y * tileSize };
}


void DevScene::UpdateLogin ( )
{
	// A-Z
	for ( int32 vk = 'A'; vk <= 'Z'; vk++ )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( vk ) ) )
		{
			if ( ( int32 ) _loginText.length ( ) < MAX_USERNAME_LEN )
				_loginText += static_cast< wchar_t >( vk - 'A' + L'a' );
		}
	}

	// 0-9
	for ( int32 vk = '0'; vk <= '9'; vk++ )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( vk ) ) )
		{
			if ( ( int32 ) _loginText.length ( ) < MAX_USERNAME_LEN )
				_loginText += static_cast< wchar_t >( vk );
		}
	}

	// Backspace
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_BACK ) ) )
	{
		if ( !_loginText.empty ( ) )
			_loginText.pop_back ( );
	}

	// Enter
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_RETURN ) ) )
	{
		if ( !_loginText.empty ( ) )
		{
			// wstring -> string
			string username ( _loginText.begin ( ) , _loginText.end ( ) );
			SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Login ( username );
			GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
		}
	}
}

void DevScene::RenderLogin ( HDC hdc )
{
	Sprite* panel = GET_SINGLE ( ResourceManager )->GetSprite ( L"LoginPanel" );
	if ( panel )
	{
		// 창 크기에 맞춰 스케일링
		::TransparentBlt (
			hdc ,
			0 , 0 ,
			GWinSizeX , GWinSizeY ,
			panel->GetDC ( ) , 
			panel->GetPos ( ).x , panel->GetPos ().y ,
			panel->GetSize ( ).x , panel->GetSize ( ).y ,
			RGB ( 0 , 0 , 0 ) // colorkey
		);
	}

	SetBkMode ( hdc , TRANSPARENT );

	int32 boxW = 300;
	int32 boxH = 150;
	int32 boxX = ( GWinSizeX - boxW ) / 2;
	int32 boxY = ( GWinSizeY - boxH ) / 2;

	// 입력 텍스트 + 커서
	SetTextColor ( hdc , RGB ( 0 , 0 , 0 ) );
	int32 fieldX = boxX + 30;
	int32 fieldY = boxY + 65;
	int32 fieldW = boxW - 60;
	int32 fieldH = 28;

	std::wstring displayText = _loginText + L"|";
	RECT textRect = { fieldX + 6, fieldY + 4, fieldX + fieldW - 6, fieldY + fieldH - 4 };
	DrawText (
		hdc ,
		displayText.c_str ( ) ,
		( int32 ) displayText.length ( ) ,
		&textRect ,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE
	);

	// 안내 문구
	SetTextColor ( hdc , RGB ( 180 , 180 , 180 ) );
	RECT hintRect = { boxX, boxY + 105, boxX + boxW, boxY + 130 };
	DrawText ( hdc , L"Enter your name and press Enter" , 31 , &hintRect , DT_CENTER );
}

void DevScene::RenderHUD ( HDC hdc )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	const int32 baseX = 36;
	const int32 baseY = 36;

	// Status Frame
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

	// Weapon Icon (선택된 무기만 표시)
	{
		Sprite* weaponSprite = nullptr;
		switch ( myPlayer->GetWeaponType ( ) )
		{
		case WeaponType::Sword:
			weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Sword_Icon" );
			break;
		case WeaponType::Bow:
			weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Bow_Icon" );
			break;
		case WeaponType::Staff:
			weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Staff_Icon" );
			break;
		}

		if ( weaponSprite )
		{
			::TransparentBlt ( hdc ,
				baseX + 0 , baseY + 0 ,
				weaponSprite->GetSize ( ).x , weaponSprite->GetSize ( ).y ,
				weaponSprite->GetDC ( ) ,
				weaponSprite->GetPos ( ).x , weaponSprite->GetPos ( ).y ,
				weaponSprite->GetSize ( ).x , weaponSprite->GetSize ( ).y ,
				weaponSprite->GetTransparent ( ) );
		}
	}

	// HP Bar
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

	// EXP Bar
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

void DevScene::CreateMapButtons ( )
{
	int32 screenW = GWinSizeX;
	int32 screenH = GWinSizeY;

	const int32 btnW = 64;
	const int32 btnH = 40;
	const int32 gap = 7;
	const int32 totalW = btnW * 3 + gap * 2;

	const int32 marginX = 10;
	const int32 marginY = 10;

	int32 startX = screenW - marginX - totalW;
	int32 y = marginY + btnH / 2;

	// 마을1 버튼
	{
		Button* b = new Button ( );
		b->SetSize ( { btnW, btnH } );
		b->SetPos ( { ( float ) ( startX + btnW / 2 ), ( float ) y } );
		b->SetSprite ( GET_SINGLE ( ResourceManager )->GetSprite ( L"Btn_Town1" ) , BS_Default );
		b->SetCurrentSprite ( b->GetSprite ( BS_Default ) );
		b->AddOnClickDelegate ( this , &DevScene::OnClickTown1 );
		_uis.push_back ( b );

		startX += btnW + gap;
	}

	// 마을2 버튼
	{
		Button* b = new Button ( );
		b->SetSize ( { btnW, btnH } );
		b->SetPos ( { ( float ) ( startX + btnW / 2 ), ( float ) y } );
		b->SetSprite ( GET_SINGLE ( ResourceManager )->GetSprite ( L"Btn_Town2" ) , BS_Default );
		b->SetCurrentSprite ( b->GetSprite ( BS_Default ) );
		b->AddOnClickDelegate ( this , &DevScene::OnClickTown2 );
		_uis.push_back ( b );

		startX += btnW + gap;
	}

	// 던전 버튼
	{
		Button* b = new Button ( );
		b->SetSize ( { btnW, btnH } );
		b->SetPos ( { ( float ) ( startX + btnW / 2 ), ( float ) y } );
		b->SetSprite ( GET_SINGLE ( ResourceManager )->GetSprite ( L"Btn_Dungeon" ) , BS_Default );
		b->SetCurrentSprite ( b->GetSprite ( BS_Default ) );
		b->AddOnClickDelegate ( this , &DevScene::OnClickDungeon );
		_uis.push_back ( b );
	}
}

void DevScene::OnClickTown1 ( )
{
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_ChangeMap ( Protocol::MAP_ID_TOWN , 1 );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void DevScene::OnClickTown2 ( )
{
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_ChangeMap ( Protocol::MAP_ID_TOWN , 2 );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void DevScene::OnClickDungeon ( )
{
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_ChangeMap ( Protocol::MAP_ID_DUNGEON , 0 );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void DevScene::HandlePartyInput ( )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	// 초대 수락/거절 (Y/N)
	if ( myPlayer->_pendingInviteFrom != 0 )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::Y ) )
		{
			SendBufferRef sb = ClientPacketHandler::Make_C_PartyAnswer ( myPlayer->_pendingInviteFrom , true );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
			myPlayer->_pendingInviteFrom = 0;
			myPlayer->_pendingInviterName.clear ( );
			GET_SINGLE ( SoundManager )->Play ( L"UISound" );
		}
		else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::N ) )
		{
			SendBufferRef sb = ClientPacketHandler::Make_C_PartyAnswer ( myPlayer->_pendingInviteFrom , false );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
			myPlayer->_pendingInviteFrom = 0;
			myPlayer->_pendingInviterName.clear ( );
			GET_SINGLE ( SoundManager )->Play ( L"UISound" );
		}
		return;  // 초대 팝업 중에는 다른 입력 무시
	}

	// P키 파티 탈퇴
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::P ) )
	{
		if ( !myPlayer->_partyMembers.empty ( ) )
		{
			SendBufferRef sb = ClientPacketHandler::Make_C_PartyLeave ( );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
		}
	}

	// 좌클릭으로 다른 플레이어 클릭 → 파티 초대
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::LeftMouse ) )
	{
		// 인벤토리 드래그 중이면 무시
		if ( _inventoryPanel && _inventoryPanel->IsVisible ( ) && _inventoryPanel->IsDragging ( ) )
			return;
		if ( _partyPanel && _partyPanel->IsVisible ( ) && _partyPanel->IsDragging ( ) )
			return;

		POINT mouse = GET_SINGLE ( InputManager )->GetMousePos ( );
		Vec2 cameraPos = GET_SINGLE ( SceneManager )->GetCameraPos ( );

		// 마우스 스크린 좌표 → 월드 좌표
		float worldX = mouse.x + cameraPos.x - GWinSizeX / 2.f;
		float worldY = mouse.y + cameraPos.y - GWinSizeY / 2.f;

		uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );

		// LAYER_OBJECT의 Player 순회
		for ( Actor* actor : _actors[ LAYER_OBJECT ] )
		{
			Player* player = dynamic_cast< Player* >( actor );
			if ( player == nullptr )
				continue;
			if ( player->info.objectid ( ) == myId )
				continue;  // 자기 자신 제외
			if ( player->info.objecttype ( ) != Protocol::OBJECT_TYPE_PLAYER )
				continue;

			Vec2 pos = player->GetPos ( );
			float dx = worldX - pos.x;
			float dy = worldY - pos.y;

			// 클릭 범위: 플레이어 중심에서 50px 이내
			if ( dx * dx + dy * dy < 50.f * 50.f )
			{
				SendBufferRef sb = ClientPacketHandler::Make_C_PartyInvite ( player->info.objectid ( ) );
				GET_SINGLE ( NetworkManager )->SendPacket ( sb );
				break;
			}
		}
	}
}

// Load functions

void DevScene::LoadSceneTextures ( )
{
	DevSceneResourceLoader::LoadSceneTextures ( );
}

void DevScene::CreateSceneSprites ( )
{
	DevSceneResourceLoader::CreateSceneSprites ( );
}

void DevScene::LoadSceneSounds ( )
{
	DevSceneResourceLoader::LoadSceneSounds ( );
}

void DevScene::AddChatMessage ( const wstring& sender , const wstring& msg )
{
	if ( _chatPanel )
	{
		_chatPanel->SetVisible ( true );
		_chatPanel->AddMessage ( sender, msg );
	}
}

void DevScene::LoadMap ( )
{
	Sprite* sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage01" );

	SpriteActor* background = new SpriteActor ( );
	background->SetSprite ( sprite );
	background->SetLayer ( LAYER_BACKGROUND );

	const Vec2Int size = sprite->GetSize ( );
	background->SetPos ( Vec2 ( size.x / 2 , size.y / 2 ) );

	AddActor ( background );
	_background = background;
}

void DevScene::LoadPlayer ( )
{
	// IDLE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleUp" );
		fb->SetInfo ( { texture, L"FB_MoveUp", {200, 200}, 0, 9, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleDown" );
		fb->SetInfo ( { texture, L"FB_MoveDown", {200, 200}, 0, 9, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleLeft" );
		fb->SetInfo ( { texture, L"FB_MoveLeft", {200, 200}, 0, 9, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleRight" );
		fb->SetInfo ( { texture, L"FB_MoveRight", {200, 200}, 0, 9, 0, 0.5f } );
	}
	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveUp" );
		fb->SetInfo ( { texture, L"FB_MoveUp", {200, 200}, 0, 9, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveDown" );
		fb->SetInfo ( { texture, L"FB_MoveDown", {200, 200}, 0, 9, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveLeft" );
		fb->SetInfo ( { texture, L"FB_MoveLeft", {200, 200}, 0, 9, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveRight" );
		fb->SetInfo ( { texture, L"FB_MoveRight", {200, 200}, 0, 9, 1, 0.5f } );
	}
	// SKILL
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackUp" );
		fb->SetInfo ( { texture, L"FB_MoveUp", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackDown" );
		fb->SetInfo ( { texture, L"FB_MoveDown", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackLeft" );
		fb->SetInfo ( { texture, L"FB_MoveLeft", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackRight" );
		fb->SetInfo ( { texture, L"FB_MoveRight", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	// BOW
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowUp" );
		fb->SetInfo ( { texture, L"FB_BowUp", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowDown" );
		fb->SetInfo ( { texture, L"FB_BowDown", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowLeft" );
		fb->SetInfo ( { texture, L"FB_BowLeft", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowRight" );
		fb->SetInfo ( { texture, L"FB_BowRight", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	// STAFF
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffUp" );
		fb->SetInfo ( { texture, L"FB_StaffUp", {200, 200}, 0, 10, 6, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffDown" );
		fb->SetInfo ( { texture, L"FB_StaffDown", {200, 200}, 0, 10, 6, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffLeft" );
		fb->SetInfo ( { texture, L"FB_StaffLeft", {200, 200}, 0, 10, 6, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffRight" );
		fb->SetInfo ( { texture, L"FB_StaffRight", {200, 200}, 0, 10, 6, 0.5f, false } );
	}

}

void DevScene::LoadMonster ( )
{
	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeUp" );
		fb->SetInfo ( { texture, L"FB_SnakeUp", {100, 100}, 0, 3, 3, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeDown" );
		fb->SetInfo ( { texture, L"FB_SnakeDown", {100, 100}, 0, 3, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeLeft" );
		fb->SetInfo ( { texture, L"FB_SnakeLeft", {100, 100}, 0, 3, 2, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeRight" );
		fb->SetInfo ( { texture, L"FB_SnakeRight", {100, 100}, 0, 3, 1, 0.5f } );
	}

	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocUp" );
		fb->SetInfo ( { texture, L"FB_OctorocUp", {100, 100}, 0, 3, 3, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocDown" );
		fb->SetInfo ( { texture, L"FB_OctorocDown", {100, 100}, 0, 3, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocLeft" );
		fb->SetInfo ( { texture, L"FB_OctorocLeft", {100, 100}, 0, 3, 2, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocRight" );
		fb->SetInfo ( { texture, L"FB_OctorocRight", {100, 100}, 0, 3, 1, 0.5f } );
	}
}

void DevScene::LoadProjectiles ( )
{
	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowUp" );
		fb->SetInfo ( { texture, L"FB_ArrowUp", {100, 100}, 0, 0, 3, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowDown" );
		fb->SetInfo ( { texture, L"FB_ArrowDown", {100, 100}, 0, 0, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowLeft" );
		fb->SetInfo ( { texture, L"FB_ArrowLeft", {100, 100}, 0, 0, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowRight" );
		fb->SetInfo ( { texture, L"FB_ArrowRight", {100, 100}, 0, 0, 2, 0.5f } );
	}
}

void DevScene::LoadEffect ( )
{
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Hit" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_Hit" );
		fb->SetInfo ( { texture, L"FB_Hit", {50, 47}, 0, 5, 0, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Explode" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_Explode" );
		fb->SetInfo ( { texture, L"FB_Explode", {144, 144}, 0, 11, 0, 0.5f, false } );
	}
}

void DevScene::LoadTilemap ( )
{
	TilemapActor* actor = new TilemapActor ( );

	actor->SetLayer ( LAYER_BACKGROUND );
	AddActor ( actor );

	_tilemapActor = actor;
	{
		auto* tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( L"Tilemap_01" );
		tm->SetMapSize ( { 63, 43 } );
		tm->SetTileSize ( 48 );

		GET_SINGLE ( ResourceManager )->LoadTilemap ( L"Tilemap_01" , L"Tilemap\\Tilemap_01.txt" );

		_tilemapActor->SetTilemap ( tm );
		_tilemapActor->SetShowDebug ( false );
	}

	{
		auto* tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( L"Tilemap_02" );
		tm->SetMapSize ( { 40, 32 } );
		tm->SetTileSize ( 48 );

		GET_SINGLE ( ResourceManager )->LoadTilemap ( L"Tilemap_02" , L"Tilemap\\Tilemap_02.txt" );
	}
}

void DevScene::LoadTilemap ( const wchar_t* tilemapFile )
{
	if ( _tilemapActor == nullptr )
	{
		TilemapActor* actor = new TilemapActor ( );
		actor->SetLayer ( LAYER_BACKGROUND );
		AddActor ( actor );
		_tilemapActor = actor;
	}

	const wchar_t* tilemapName = nullptr;
	Vec2Int mapSize = {};

	if ( wcscmp ( tilemapFile , L"Tilemap\\Tilemap_01.txt" ) == 0 )
	{
		tilemapName = L"Tilemap_01";
		mapSize = { 63, 43 };
	}
	else if ( wcscmp ( tilemapFile , L"Tilemap\\Tilemap_02.txt" ) == 0 )
	{
		tilemapName = L"Tilemap_02";
		mapSize = { 40, 32 };
	}
	else
	{
		return;
	}

	Tilemap* tm = GET_SINGLE ( ResourceManager )->GetTilemap ( tilemapName );
	if ( tm == nullptr )
	{
		tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( tilemapName );
		if ( tm == nullptr )
			return;

		tm->SetTileSize ( 48 );
		tm->SetMapSize ( mapSize );
		GET_SINGLE ( ResourceManager )->LoadTilemap ( tilemapName , tilemapFile );
	}

	_tilemapActor->SetTilemap ( tm );
	_tilemapActor->SetShowDebug ( false );
}

