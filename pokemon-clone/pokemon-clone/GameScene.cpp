#include "pch.h"
#include "GameScene.h"

#include "ObjectManager.h"

#include "Object.h"
#include "Player.h"
#include "Monster.h"

GameScene::GameScene ( )
{
}

GameScene::~GameScene ( )
{
}

void GameScene::Init ( )
{
	Monster* monster = GET_SINGLE ( ObjectManager )->CreateObject<Monster> ( );
	monster->SetPos ( Pos{ 400, 400 } );
	GET_SINGLE ( ObjectManager )->Add ( monster );
}

void GameScene::Update ( )
{
	const vector<Object*> objects = GET_SINGLE ( ObjectManager )->GetObjects ( );
	for ( Object* object : objects )
	{
		object->Update ( );
	}
}

void GameScene::Render ( HDC hdc )
{
	const vector<Object*>& objects = GET_SINGLE ( ObjectManager )->GetObjects ( );
	for ( Object* object : objects )
	{
		object->Render ( hdc );
	}
}
