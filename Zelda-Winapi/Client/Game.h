#pragma once

class Game
{
public:
	Game ( );
	~Game ( );

public:
	void Init (HWND hwnd, uint16 port = 7777);
	void Update ( );
	void Render ( );

private:
	HWND _hwnd = {};
	HDC _hdc = {};

private:
	// Double Buffering
	RECT _rect;
	HDC _hdcBack = {};
	HBITMAP _bmpBack = {};
};

