#pragma once
#include "Object.h"
class Player : public Object
{
public:
	Player ();
	virtual ~Player ( ) override;

	virtual void Init ( ) override;
	virtual void Update ( ) override;
	virtual void Render ( HDC hdc ) override;

	Pos GetFirePos ( );

public:
	float _barrelAngle = 0.f;
	float _barrelLength = 100.f;
};

