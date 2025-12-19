#pragma once

class Actor
{
public:
	Actor ( );
	virtual ~Actor ( );

	virtual void BeginPlay ( ) ;
	virtual void Tick ( ) ;
	virtual void Render ( HDC hdc ) ;

	void SetPos ( Vec2 pos ) { _pos = pos; }
	Vec2 GetPos ( ) { return _pos; }

protected:
	Vec2 _pos = { 0, 0 };
};

