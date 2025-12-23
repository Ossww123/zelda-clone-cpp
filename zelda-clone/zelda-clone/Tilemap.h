#pragma once
#include "ResourceBase.h"

struct Tile
{
	// TODO
	int32 value = 0;
};

enum TILE_SIZE
{
	TILE_WIDTH = 40,
	TILE_HEIGHT = 32,
	TILE_SIZEX = 48,
	TILE_SIZEY = 48
};

class Tilemap : public ResourceBase
{
public:
	Tilemap ( );
	virtual ~Tilemap ( ) override;

	virtual void LoadFile ( const wstring& path ) override;
	virtual void SaveFile ( const wstring& psyh ) override;

	Vec2Int GetMapSize ( ) { return _mapSize; }
	int32 GetTileSize ( ) { return _tileSize; }
	Tile* GetTileAt ( Vec2Int pos );
	vector<vector<Tile>>& GetTiles ( ) { return _tiles; }

	void SetMapSize ( Vec2Int size );
	void SetTileSize ( int32 size );

private:
	Vec2Int _mapSize = { };
	int32 _tileSize = 0;
	vector<vector<Tile>> _tiles;
};

