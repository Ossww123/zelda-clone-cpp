#include "pch.h"
#include "Game.h"

#include "TimeManager.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "NetworkManager.h"

Game::Game ( )
{
}

Game::~Game ( )
{
	GET_SINGLE ( SceneManager )->Clear ( );
	GET_SINGLE ( ResourceManager )->Clear ( );

	_CrtDumpMemoryLeaks ( );
}

void Game::Init ( HWND hwnd )
{
	_hwnd = hwnd;
	_hdc = ::GetDC ( hwnd );

	::GetClientRect(hwnd , & _rect);

	_hdcBack = ::CreateCompatibleDC ( _hdc );
	_bmpBack = ::CreateCompatibleBitmap ( _hdc , _rect.right , _rect.bottom );
	HBITMAP prev = ( HBITMAP )::SelectObject ( _hdcBack , _bmpBack );
	::DeleteObject ( prev );



	GET_SINGLE ( TimeManager  )->Init ( );
	GET_SINGLE ( InputManager )->Init ( hwnd );
	GET_SINGLE ( SceneManager )->Init ( );

	wchar_t buffer[ MAX_PATH ];
	GetModuleFileNameW ( nullptr , buffer , MAX_PATH );

	fs::path exePath ( buffer );
	fs::path resourcePath = exePath.parent_path ( ) / L"..\\..\\..\\Resources";
	resourcePath = fs::weakly_canonical ( resourcePath );

	GET_SINGLE ( ResourceManager )->Init ( hwnd , resourcePath );
	GET_SINGLE ( SoundManager )->Init ( hwnd );
	GET_SINGLE ( SceneManager )->ChangeScene ( SceneType::DevScene );
	GET_SINGLE ( NetworkManager )->Init ( );
}

void Game::Update ( )
{
	GET_SINGLE ( TimeManager  )->Update ( );
	GET_SINGLE ( InputManager )->Update ( );
	GET_SINGLE ( SceneManager )->Update ( );
	GET_SINGLE ( NetworkManager )->Update ( );
}

void Game::Render ( )
{
	uint32 fps = GET_SINGLE ( TimeManager )->GetFPS ( );
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	GET_SINGLE ( SceneManager )->Render ( _hdcBack );

	if(false )
	{
		POINT mousePos = GET_SINGLE ( InputManager )->GetMousePos ( );
		wstring str = std::format ( L"Mouse({0}), ({1})" , mousePos.x , mousePos.y );
		::TextOut ( _hdcBack , 20 , 10 , str.c_str ( ) , static_cast< int32 >( str.size ( ) ) );
	}

	if(false )
	{
		wstring str = std::format ( L"FPS({0}), DT({1} ms)" , fps , static_cast< int32 >( deltaTime * 1000 ) );
		::TextOut ( _hdcBack , 650 , 10 , str.c_str ( ) , static_cast< int32 >( str.size ( ) ) );
	}

	// Double Buffering
	::BitBlt ( _hdc , 0 , 0 , _rect.right , _rect.bottom , _hdcBack , 0 , 0 , SRCCOPY );
	::PatBlt ( _hdcBack , 0 , 0 , _rect.right , _rect.bottom , WHITENESS );
}
