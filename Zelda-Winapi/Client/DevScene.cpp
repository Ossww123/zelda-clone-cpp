#include "pch.h"
#include "DevScene.h"

#include "Utils.h"
#include "Texture.h"
#include "Sprite.h"
#include "Actor.h"
#include "SpriteActor.h"
#include "Flipbook.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
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
#include "CollisionManager.h"
#include "SoundManager.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"

DevScene::DevScene ( )
{
}

DevScene::~DevScene ( )
{
}

void DevScene::Init ( )
{
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Stage01" , L"Sprite\\Map\\Stage01.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Stage02" , L"Sprite\\Map\\Stage02.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Tile" , L"Sprite\\Map\\Tile.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Sword" , L"Sprite\\Item\\Sword.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Arrow" , L"Sprite\\Item\\Arrow.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Potion" , L"Sprite\\UI\\Mp.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerDown" , L"Sprite\\Player\\PlayerDown.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerUp" , L"Sprite\\Player\\PlayerUp.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerLeft" , L"Sprite\\Player\\PlayerLeft.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerRight" , L"Sprite\\Player\\PlayerRight.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Snake" , L"Sprite\\Monster\\Snake.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Hit" , L"Sprite\\Effect\\Hit.bmp" , RGB ( 0 , 0 , 0 ) );

	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Start" , L"Sprite\\UI\\Start.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Edit" , L"Sprite\\UI\\Edit.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Exit" , L"Sprite\\UI\\Exit.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"MapButton" , L"Sprite\\UI\\MapButton.bmp" , RGB ( 211 , 249 , 188 ) );

	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Stage01" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Stage01" ) , 0 , 0 , 0 , 0 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Stage02" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Stage02" ) , 0 , 0 , 0 , 0 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"TileO" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Tile" ) , 0 , 0 , 48 , 48 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"TileX" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Tile" ) , 48 , 0 , 48 , 48 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" ) , 0 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" ) , 150 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Edit_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Edit" ) , 0 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Edit_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Edit" ) , 150 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Exit_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Exit" ) , 0 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Exit_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Exit" ) , 150 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Btn_Town1" , GET_SINGLE ( ResourceManager )->GetTexture ( L"MapButton" ) , 0 , 0 , 64 , 40 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Btn_Town2" , GET_SINGLE ( ResourceManager )->GetTexture ( L"MapButton" ) , 71 , 0 , 64 , 40 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Btn_Dungeon" , GET_SINGLE ( ResourceManager )->GetTexture ( L"MapButton" ) , 142 , 0 , 64 , 40 );


	LoadMap ( );
	LoadPlayer ( );
	LoadMonster ( );
	LoadProjectiles ( );
	LoadEffect ( );
	LoadTilemap ( );

	if (false)
	{
		GET_SINGLE ( ResourceManager )->LoadSound ( L"BGM" , L"Sound\\BGM.wav" );
		GET_SINGLE ( ResourceManager )->LoadSound ( L"Attack" , L"Sound\\Sword.wav" );
	}

	CreateMapButtons ( );
	Super::Init ( );
}

void DevScene::Update ( )
{
	Super::Update ( );

	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	TickMonsterSpawn ( );
}

void DevScene::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void DevScene::AddActor ( Actor* actor )
{
	Super::AddActor ( actor );

	Monster* monster = dynamic_cast< Monster* >( actor );
	if ( monster )
	{
		_monsterCount++;
	}
}

void DevScene::RemoveActor ( Actor* actor )
{
	Super::RemoveActor ( actor );

	Monster* monster = dynamic_cast< Monster* >( actor );
	if ( monster )
	{
		_monsterCount--;
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



void DevScene::ChangeMap ( Protocol::MAP_ID mapId )
{
	ClearWorldActors ( );
	ChangeBackground ( mapId );

	// 카운트 리셋
	_monsterCount = 0;

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
			monster->SetDir ( info.dir ( ) );
			monster->SetState ( info.state ( ) );
			monster->info = info;
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

bool DevScene::FindPath ( Vec2Int src , Vec2Int dest , vector<Vec2Int>& path , int32 maxDepth )
{
	int32 depth = abs ( dest.y - src.y ) + abs ( dest.x - src.x );
	if ( depth >= maxDepth )
		return false;

	priority_queue<PQNode , vector<PQNode> , greater<PQNode>> pq;
	map<Vec2Int , int32> best;
	map<Vec2Int , Vec2Int> parent;

	// 초기값
	{
		int32 cost = abs ( dest.y - src.y ) + abs ( dest.x - src.x );

		pq.push ( PQNode ( cost , src ) );
		best[ src ] = cost;
		parent[ src ] = src;
	}

	Vec2Int front[ 4 ] =
	{
		{0, -1},
		{0, 1},
		{-1, 0},
		{1, 0},
	};

	bool found = false;

	while ( pq.empty ( ) == false )
	{
		// 제일 좋은 후보를 찾는다
		PQNode node = pq.top ( );
		pq.pop ( );

		// 더 짧은 경로를 뒤늦게 찾았다면 스킵
		if ( best[ node.pos ] < node.cost )
			continue;

		// 목적지에 도착했으면 바로 종료
		if ( node.pos == dest )
		{
			found = true;
			break;
		}

		// 방문
		for ( int32 dir = 0; dir < 4; dir++ )
		{
			Vec2Int nextPos = node.pos + front[ dir ];

			if ( CanGo ( nextPos ) == false )
				continue;

			int32 depth = abs ( nextPos.y - src.y ) + abs ( nextPos.x - src.x );
			if ( depth >= maxDepth )
				continue;

			int32 cost = abs ( dest.y - nextPos.y ) + abs ( dest.x - nextPos.x );
			int32 bestCost = best[ nextPos ];
			if ( bestCost != 0 )
			{
				// 다른 경로에서 더 빠른 길을 찾았으면 스킵
				if ( bestCost <= cost )
					continue;
			}

			// 예약 진행
			best[ nextPos ] = cost;
			pq.push ( PQNode ( cost , nextPos ) );
			parent[ nextPos ] = node.pos;
		}
	}

	if ( found == false )
	{
		// TODO
		float bestScore = FLT_MAX;

		for ( auto& item : best )
		{
			Vec2Int pos = item.first;
			int32 score = item.second;

			// 동점이라면, 최초 위치에서 가장 덜 이동하는 쪽으로
			if ( bestScore == score )
			{
				int32 dist1 = abs ( dest.x - src.x ) + abs ( dest.y - src.y );
				int32 dist2 = abs ( pos.x - src.x ) + abs ( pos.y - src.y );
				if ( dist1 > dist2 )
					dest = pos;
			}
			else if ( bestScore > score )
			{
				dest = pos;
				bestScore = score;
			}
		}
	}

	path.clear ( );
	Vec2Int pos = dest;

	while ( true )
	{
		path.push_back ( pos );

		// 시작점
		if ( pos == parent[ pos ] )
			break;

		pos = parent[ pos ];
	}

	std::reverse ( path.begin ( ) , path.end ( ) );
	return true;
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

Vec2Int DevScene::GetRandomEmptyCellPos ( )
{
	Vec2Int ret = { -1, -1 };

	if ( _tilemapActor == nullptr )
		return ret;

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return ret;

	Vec2Int size = tm->GetMapSize ( );

	while ( true )
	{
		int32 x = rand ( ) % size.x;
		int32 y = rand ( ) % size.y;
		Vec2Int cellPos ( x , y );

		if ( CanGo ( cellPos ) )
			return cellPos;
	}
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


void DevScene::TickMonsterSpawn ( )
{
	return;


	// 레거시 코드 //
	if ( _monsterCount < DESIRED_MONSTER_COUNT )
		SpawnObjectAtRandomPos<Monster> ( );
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
