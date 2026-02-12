#pragma once
#include "GameObject.h"

class Effect : public GameObject
{
	using Super = GameObject;

public:
	Effect ( );

	virtual ~Effect ( ) override = default;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

protected:
	void RemoveFromScene ( );
};
