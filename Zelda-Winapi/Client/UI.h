#pragma once


class UI
{
public:
	UI ( );
	virtual ~UI ( );

	virtual void BeginPlay ( );
	virtual void Tick ( );
	virtual void Render ( HDC hdc );

	void SetPos ( Vec2 pos ) { _pos = pos; }
	Vec2 GetPos ( ) { return _pos; }
	void SetSize ( Vec2Int size ) { _size = size; }
	Vec2Int GetSize ( ) { return _size; }

	virtual void SetVisible ( bool visible ) { _visible = visible; }
	bool IsVisible ( ) const { return _visible; }

	void SetEnabled ( bool enabled ) { _enabled = enabled; }
	bool IsEnabled ( ) const { return _enabled; }

	RECT GetRect ( );
	bool IsMouseInRect ( );

protected:
	Vec2 _pos = { 400, 300 };
	Vec2Int _size = { 150, 150 };
	bool _visible = true;
	bool _enabled = true;
};

