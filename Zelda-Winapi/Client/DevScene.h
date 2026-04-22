#pragma once
#include "Scene.h"
#include "UIManager.h"

class Actor;
class Player;
class GameObject;
class UI;
class SpriteActor;
class Sprite;

class DevScene : public Scene
{
	using Super = Scene;
public:
	DevScene ( );
	virtual ~DevScene ( ) override;

	virtual void Init ( ) override;
	virtual void Update ( ) override;
	virtual void Render ( HDC hdc ) override;

	virtual void AddActor ( Actor* actor ) override;
	virtual void RemoveActor ( Actor* actor ) override;

	template<typename T>
	T* SpawnObject ( Vec2Int pos )
	{
		auto isGameObject = std::is_convertible_v<T* , GameObject*>;
		assert ( isGameObject );

		T* ret = new T ( );
		ret->SetCellPos ( pos , true );
		AddActor ( ret );
		ret->BeginPlay ( );

		return ret;
	}

public:
	void ChangeMap ( Protocol::MAP_ID mapId );
	void ChangeBackground ( Protocol::MAP_ID mapId );

public:
	void Handle_S_AddObject ( Protocol::S_AddObject& pkt );
	void Handle_S_RemoveObject ( Protocol::S_RemoveObject& pkt );

public:
	GameObject* GetObject ( uint64 id );
	Player* FindClosestPlayer ( Vec2Int cellPos );

	bool CanGo ( Vec2Int cellPos );
	Vec2 ConvertPos ( Vec2Int cellPos );

public:
	Vec2Int GetWorldPixelSize ( ) const;
	bool HasMapId ( ) const { return _hasMapId; }
	bool IsTown ( ) const { return ( _hasMapId && _currentMapId == Protocol::MAP_ID_TOWN ); }

	void SetLoggedIn ( bool value ) { _loggedIn = value; }

	void AddChatMessage ( const wstring& sender , const wstring& msg ) { _uiManager.AddChatMessage ( sender , msg ); }
	void SetRankingData ( const vector<pair<wstring, int32>>& entries ) { _uiManager.SetRankingData ( entries ); }

private:
	void LoadMap ( );
	void LoadPlayer ( );
	void LoadMonster ( );
	void LoadProjectiles ( );
	void LoadEffect ( );
	void LoadTilemap ( );
	void LoadTilemap ( const wchar_t* tilemapFile );
	void LoadSceneTextures ( );
	void CreateSceneSprites ( );
	void LoadSceneSounds ( );

private:
	void UpdateLogin ( );
	void RenderLogin ( HDC hdc );
	void HandlePartyInviteClick ( );

private:
	void CreateMapButtons ( );
	void OnClickTown1 ( );
	void OnClickTown2 ( );
	void OnClickDungeon ( );

private:
	bool _loggedIn = false;
	wstring _loginText;
	static const int32 MAX_USERNAME_LEN = 12;

	UIManager _uiManager;

	class TilemapActor* _tilemapActor = nullptr;
	SpriteActor* _background = nullptr;
	Protocol::MAP_ID _currentMapId = Protocol::MAP_ID_NONE;
	bool _hasMapId = false;
};
