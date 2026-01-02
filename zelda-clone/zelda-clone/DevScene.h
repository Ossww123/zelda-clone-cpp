#pragma once
#include "Scene.h"

class Actor;
class GameObject;
class UI;

class DevScene : public Scene
{
	using Super = Scene;
public:
	DevScene ( );
	virtual ~DevScene ( ) override;

	virtual void Init ( ) override;
	virtual void Update ( ) override;
	virtual void Render ( HDC hdc ) override;

	void LoadMap ( );
	void LoadPlayer ( );
	void LoadMonster ( );
	void LoadProjectiles ( );
	void LoadEffect ( );
	void LoadTilemap ( );

	template<typename T>
	T* SpawnObject ( Vec2Int pos )
	{
		// Type-Trait
		auto isGameObject = std::is_convertible_v<T* , GameObject*>;
		assert ( isGameObject );

		T* ret = new T ( );
		ret->SetCellPos ( pos , true );
		AddActor ( ret );

		ret->BeginPlay ( );

		return ret;
	}

	bool CanGo ( Vec2Int cellPos );
	Vec2 ConvertPos ( Vec2Int cellPos );

	class TilemapActor* _tilemapActor = nullptr;
};

