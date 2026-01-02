#pragma once
#include "GameObject.h"

class Creature : public GameObject
{
	using Super = GameObject;

public:
	Creature ( );
	virtual ~Creature ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:

	virtual void TickIdle ( ) override {};
	virtual void TickMove ( ) override {};
	virtual void TickSkill ( ) override {};
	virtual void UpdateAnimation ( ) override {};

	void SetStatus ( Status status ) { _status = status; }
	Status& GetStatus ( ) { return _status; }

protected:
	Status _status;
};

