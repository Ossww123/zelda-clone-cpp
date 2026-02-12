#pragma once
#include "Effect.h"

class ExplodeEffect : public Effect
{
	using Super = Effect;

public:
	ExplodeEffect ( );
	virtual ~ExplodeEffect ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:
	virtual void UpdateAnimation ( ) override;
};

