#include "pch.h"
#include "DevScene.h"

#include "Utils.h"
#include "Texture.h"
#include "Sprite.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"

DevScene::DevScene ( )
{
}

DevScene::~DevScene ( )
{
}

void DevScene::Init ( )
{
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Stage01" , L"Sprite\\Map\\Stage01.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Sword" , L"Sprite\\Item\\Sword.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Potion" , L"Sprite\\UI\\Mp.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerDown" , L"Sprite\\Player\\PlayerUp.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerDown" , L"Sprite\\Player\\PlayerDown.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerDown" , L"Sprite\\Player\\PlayerLeft.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"PlayerDown" , L"Sprite\\Player\\PlayerRight.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Start" , L"Sprite\\UI\\Start.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Edit" , L"Sprite\\UI\\Edit.bmp" );
	GET_SINGLE ( ResourceManager )->LoadTexture ( L"Exit" , L"Sprite\\UI\\Exit.bmp" );

	Texture* tex = GET_SINGLE ( ResourceManager )->GetTexture ( L"Start" );
	GET_SINGLE ( ResourceManager )->CreateSprite ( L"Start_On" , tex , 128 , 0 , 128 , 128 );
}

void DevScene::Update ( )
{
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );
}

void DevScene::Render ( HDC hdc )
{
	// Texture* tex = GET_SINGLE ( ResourceManager )->GetTexture ( L"Stage01" );

	// ::BitBlt ( hdc , 0 , 0 , GWinSizeX , GWinSizeY , tex->GetDC ( ) , 0 , 0 , SRCCOPY );


	Sprite* sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Start_On" );
	::BitBlt ( hdc ,
		0 ,
		0 ,
		GWinSizeX ,
		GWinSizeY ,
		sprite->GetDC ( ) ,
		sprite->GetPos ( ).x ,
		sprite->GetPos ( ).y ,
		SRCCOPY );
}
