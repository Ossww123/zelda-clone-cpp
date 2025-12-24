#include "pch.h"
#include "DevScene.h"

#include "Utils.h"
#include "Texture.h"
#include "Sprite.h"
#include "Actor.h"
#include "SpriteActor.h"
#include "Flipbook.h"
#include "SphereCollider.h"
#include "Player.h"
#include "UI.h"
#include "Button.h"
#include "Tilemap.h"
#include "TilemapActor.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "CollisionManager.h"


DevScene::DevScene ( )
{
}

DevScene::~DevScene ( )
{
}

void DevScene::Init ( )
{
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Stage01" , L"Sprite\\Map\\Stage01.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Tile" , L"Sprite\\Map\\Tile.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Sword" , L"Sprite\\Item\\Sword.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Potion" , L"Sprite\\UI\\Mp.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerUp" , L"Sprite\\Player\\PlayerUp.bmp" , RGB(128, 128, 128));
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerDown" , L"Sprite\\Player\\PlayerDown.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerLeft" , L"Sprite\\Player\\PlayerLeft.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerRight" , L"Sprite\\Player\\PlayerRight.bmp" , RGB ( 128 , 128 , 128 ) );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Start" , L"Sprite\\UI\\Start.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Edit" , L"Sprite\\UI\\Edit.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Exit" , L"Sprite\\UI\\Exit.bmp" );

	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Stage01" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Stage01" ) , 0, 0, 0, 0);
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"TileO" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Tile" ) , 0 , 0 , 48 , 48 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"TileX" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Tile" ) , 48 , 0 , 48 , 48 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" ) , 0 , 0 , 128 , 128 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" ) , 128 , 0 , 128 , 128 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Edit_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Edit" ) , 0 , 0 , 128 , 128 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Edit_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Edit" ) , 128 , 0 , 128 , 128 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Exit_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Exit" ) , 0 , 0 , 128 , 128 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Exit_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Exit" ) , 128 , 0 , 128 , 128 );

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

	{
		Sprite* sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage01" );

		SpriteActor* background = new SpriteActor ( );
		background->SetSprite ( sprite );
		background->SetLayer ( LAYER_BACKGROUND );
		const Vec2Int size = sprite->GetSize ( );
		background->SetPos (Vec2(size.x / 2,size.y / 2) );

		AddActor ( background );
	}

	{
		Player* player = new Player ( );

		/*{
			SphereCollider* collider = new SphereCollider ( );
			collider->SetRadius ( 100.f );
			player->AddComponent ( collider );
			GET_SINGLE ( CollisionManager )->AddCollider ( collider );
		}*/
		AddActor ( player );
	}

	/*{
		Actor* player = new Actor ( );

		{
			SphereCollider* collider = new SphereCollider ( );
			collider->SetRadius ( 50.f );
			player->AddComponent ( collider );
			GET_SINGLE ( CollisionManager )->AddCollider ( collider );
			player->SetPos ( { 400, 200 });
		}
		AddActor ( player );
	}*/

	{
		TilemapActor* actor = new TilemapActor ( );
		AddActor ( actor );

		_tilemapActor = actor;
		{
			auto* tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( L"Tilemap_01" );
			tm->SetMapSize ( { 40, 32 } );
			tm->SetTileSize ( 64 );

			_tilemapActor->SetTilemap ( tm );
			_tilemapActor->SetShowDebug ( true );
		}
	}

	Super::Init ( );
}

void DevScene::Update ( )
{
	Super::Update ( );

	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::Q ) )
	{
		GET_SINGLE ( ResourceManager )->SaveTilemap ( L"Tilemap_01" , L"Tilemap\\Tilemap01.txt" );
	}
	else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::E ) )
	{
		GET_SINGLE ( ResourceManager )->LoadTilemap ( L"Tilemap_01" , L"Tilemap\\Tilemap01.txt" );
	}
}

void DevScene::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void DevScene::Clear ( )
{
	for ( const vector<Actor*>& actors : _actors )
		for ( Actor* actor : actors )
			SAFE_DELETE ( actor );

	for ( vector<Actor*>& actors : _actors )
		actors.clear ( );

	for ( UI* ui : _uis )
		SAFE_DELETE ( ui );

	_uis.clear ( );
}
