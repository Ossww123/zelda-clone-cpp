#pragma once

class ResourceBase;
class Texture;
class Sprite;
class Flipbook;

class ResourceManager
{
	DECLARE_SINGLE ( ResourceManager );
public:
	~ResourceManager ( );

public:
	void Init (HWND hwnd, fs::path resourcePath );
	void Clear ( );

	const fs::path& GetResourcePath ( ) { return _resourcePath; }

	Texture* GetTexture ( const wstring& key ) { return _textures[ key ]; }
	Texture* LoadTexture ( const wstring& key , const wstring&& path , uint32 transparent = RGB ( 255 , 0 , 255 ) );
	
	Sprite* GetSprite ( const wstring& key ) { return _sprites[ key ]; }
	Sprite* CreateSprite ( const wstring& key , Texture* texture , int32 x , int32 y , int32 cx , int32 cy );

	Flipbook* GetFlipbook ( const wstring& key ) { return _flipbooks[ key ]; }
	Flipbook* CreateFlipbook ( const wstring& key);

private:
	HWND _hwnd;
	fs::path _resourcePath;

	unordered_map<wstring , Texture*> _textures;
	unordered_map<wstring , Sprite*> _sprites;
	unordered_map<wstring , Flipbook*> _flipbooks;
};

