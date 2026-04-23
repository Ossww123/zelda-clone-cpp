#include "pch.h"
#include "DevScene.h"

#include "Client.h"
#include "Utils.h"
#include "Sprite.h"
#include "Actor.h"
#include "SpriteActor.h"
#include "Player.h"
#include "Arrow.h"
#include "Tilemap.h"
#include "TilemapActor.h"
#include "Sound.h"
#include "Monster.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"
#include "DevSceneResourceLoader.h"
#include "SoundManager.h"
#include "MyPlayer.h"
#include "ExplodeEffect.h"
#include "Flipbook.h"

DevScene::DevScene ( )
{
}

DevScene::~DevScene ( )
{
}

void DevScene::Init ( )
{
	DevSceneResourceLoader::LoadSceneTextures ( );
	DevSceneResourceLoader::CreateSceneSprites ( );
	DevSceneResourceLoader::LoadFlipbooks ( );
	LoadMap ( );
	LoadTilemap ( );

	_currentMapId = Protocol::MAP_ID_TOWN;
	_hasMapId = true;

	DevSceneResourceLoader::LoadSceneSounds ( );

	_uiManager.Init ( g_hWnd );

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

	_uiManager.Tick ( );

	if ( !_uiManager.IsInputConsumed ( ) )
		HandlePartyInviteClick ( );
}

void DevScene::Render ( HDC hdc )
{
	if ( !_loggedIn )
	{
		RenderLogin ( hdc );
		return;
	}

	Super::Render ( hdc );
	_uiManager.Render ( hdc );
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

void DevScene::Handle_S_Attack ( Protocol::S_Attack& pkt )
{
	switch ( pkt.weapontype ( ) )
	{
	case Protocol::WEAPON_TYPE_SWORD:
		GET_SINGLE ( SoundManager )->Play ( L"Sword" );
		break;
	case Protocol::WEAPON_TYPE_BOW:
		GET_SINGLE ( SoundManager )->Play ( L"Arrow" );
		break;
	case Protocol::WEAPON_TYPE_STAFF:
		GET_SINGLE ( SoundManager )->Play ( L"Explode" );
		break;
	default:
		break;
	}

	GameObject* gameObject = GetObject ( pkt.attackerid ( ) );
	if ( gameObject )
	{
		gameObject->SetDir ( pkt.dir ( ) );

		if ( Player* player = dynamic_cast< Player* >( gameObject ) )
		{
			auto weapon = Player::FromProtoWeaponType ( pkt.weapontype ( ) );
			player->SetWeaponType ( weapon );

			float duration = 0.35f;
			if ( Flipbook* fb = player->GetSkillFlipbook ( weapon , pkt.dir ( ) ) )
				duration = fb->GetInfo ( ).duration;

			gameObject->StartSkillAnim ( duration );
		}
		else
		{
			gameObject->StartSkillAnim ( 0.35f );
		}

		if ( pkt.weapontype ( ) == Protocol::WEAPON_TYPE_STAFF )
		{
			Vec2Int forward{ 0 , 0 };
			switch ( pkt.dir ( ) )
			{
			case Protocol::DIR_TYPE_UP:    forward = { 0 , -2 }; break;
			case Protocol::DIR_TYPE_DOWN:  forward = { 0 ,  2 }; break;
			case Protocol::DIR_TYPE_LEFT:  forward = { -2 , 0 }; break;
			case Protocol::DIR_TYPE_RIGHT: forward = {  2 , 0 }; break;
			}
			SpawnObject<ExplodeEffect> ( gameObject->GetCellPos ( ) + forward );
		}
	}

	uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );
	if ( pkt.attackerid ( ) == myId )
	{
		if ( MyPlayer* mp = GET_SINGLE ( SceneManager )->GetMyPlayer ( ) )
			mp->OnServerAttackAck ( );
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
			Protocol::C_Login loginPkt;
			loginPkt.set_username ( username );
			SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Login ( loginPkt );
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


void DevScene::HandlePartyInviteClick ( )
{
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::LeftMouse ) == false )
		return;

	POINT mouse = GET_SINGLE ( InputManager )->GetMousePos ( );
	Vec2 cameraPos = GET_SINGLE ( SceneManager )->GetCameraPos ( );

	float worldX = mouse.x + cameraPos.x - GWinSizeX / 2.f;
	float worldY = mouse.y + cameraPos.y - GWinSizeY / 2.f;

	uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );

	for ( Actor* actor : _actors[ LAYER_OBJECT ] )
	{
		Player* player = dynamic_cast< Player* >( actor );
		if ( player == nullptr )
			continue;
		if ( player->info.objectid ( ) == myId )
			continue;
		if ( player->info.objecttype ( ) != Protocol::OBJECT_TYPE_PLAYER )
			continue;

		Vec2 pos = player->GetPos ( );
		float dx = worldX - pos.x;
		float dy = worldY - pos.y;

		if ( dx * dx + dy * dy < 50.f * 50.f )
		{
			Protocol::C_PartyInvite invitePkt;
			invitePkt.set_targetid ( player->info.objectid ( ) );
			GET_SINGLE ( NetworkManager )->SendPacket ( ClientPacketHandler::Make_C_PartyInvite ( invitePkt ) );
			break;
		}
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

