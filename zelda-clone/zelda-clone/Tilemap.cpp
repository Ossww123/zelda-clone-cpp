#include "pch.h"
#include "Tilemap.h"

Tilemap::Tilemap ( )
{
}

Tilemap::~Tilemap ( )
{
}

void Tilemap::LoadFile ( const wstring& path )
{
}

void Tilemap::SaveFile ( const wstring& psyh )
{
}

Tile* Tilemap::GetTileAt ( Vec2Int pos )
{
	if ( pos.x < 0 || pos.x >= _mapSize.x || pos.y < 0 || pos.y >= _mapSize.y )
		return nullptr;

	return &_tiles[ pos.y ][ pos.x ];
}

void Tilemap::SetMapSize ( Vec2Int size )
{
	_mapSize = size;

	_tiles = vector<vector<Tile>> ( size.y , vector<Tile> ( size.x ) );

	for ( int32 y = 0; y < size.y; y++ )
	{
		for ( int32 x = 0; x < size.x; x++ )
		{
			_tiles[ y ][ x ] = Tile{ 0 };
		}
	}
}

void Tilemap::SetTileSize ( int32 size )
{
	_tileSize = size;
}
