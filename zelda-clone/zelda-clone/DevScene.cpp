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

	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Stage01" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Stage01" ) , 0 , 0 , 0 , 0 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"TileO" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Tile" ) , 0 , 0 , 48 , 48 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"TileX" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Tile" ) , 48 , 0 , 48 , 48 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" ) , 0 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" ) , 150 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Edit_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Edit" ) , 0 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Edit_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Edit" ) , 150 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Exit_Off" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Exit" ) , 0 , 0 , 150 , 150 );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Exit_On" , GET_SINGLE ( ResourceManager )->GetTexture ( L"Exit" ) , 150 , 0 , 150 , 150 );

	LoadMap ( );
	LoadPlayer ( );
	LoadMonster ( );
	LoadProjectiles ( );
	LoadEffect ( );
	LoadTilemap ( );

	//GET_SINGLE ( ResourceManager )->LoadSound ( L"BGM" , L"Sound\\BGM.wav" );
	//GET_SINGLE ( ResourceManager )->LoadSound ( L"Attack" , L"Sound\\Sword.wav" );

	SpawnObject<Player> ( Vec2Int{ 5 , 5 } );
	SpawnObject<Monster> ( Vec2Int{ 7 , 7 } );

	Super::Init ( );
}

void DevScene::Update ( )
{
	Super::Update ( );

	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	/*if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::Q ) )
	{
		GET_SINGLE ( ResourceManager )->SaveTilemap ( L"Tilemap_01" , L"Tilemap\\Tilemap01.txt" );
	}
	else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::E ) )
	{
		GET_SINGLE ( ResourceManager )->LoadTilemap ( L"Tilemap_01" , L"Tilemap\\Tilemap01.txt" );
	}*/
}

void DevScene::Render ( HDC hdc )
{
	Super::Render ( hdc );
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
