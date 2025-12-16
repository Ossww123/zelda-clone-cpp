#include "pch.h"
#include "Game.h"

#include "TimeManager.h"

Game::Game ( )
{
}

Game::~Game ( )
{
}

void Game::Init ( HWND hwnd )
{
	_hwnd = hwnd;
	_hdc = ::GetDC ( hwnd );

	GET_SINGLE ( TimeManager )->Init ( );
}

void Game::Update ( )
{
	GET_SINGLE ( TimeManager )->Update ( );
}

void Game::Render ( )
{
	uint32 fps = GET_SINGLE ( TimeManager )->GetFPS ( );
	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	wstring str = std::format(L"FPS({0}), DT({1} ms)", fps, static_cast<int32>(deltaTime * 1000));
	::TextOut ( _hdc , 650 , 10 , str.c_str(), static_cast<int32>(str.size()));

	::Rectangle ( _hdc , 200 , 200 , 400 , 400 );
}
