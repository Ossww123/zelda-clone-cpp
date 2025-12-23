#pragma once
#include "Scene.h"

class Actor;
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

	void Clear ( );

	class TilemapActor* _tilemapActor = nullptr;
};

