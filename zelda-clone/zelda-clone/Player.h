#pragma once
#include "FlipbookActor.h"

class Flipbook;
class Collider;

class Player : public FlipbookActor
{
	using Super = FlipbookActor;

public:
	Player ( );
	virtual ~Player ( ) override;

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

	virtual void OnComponentBeginOverlap ( Collider* collider , Collider* other ) override;
	virtual void OnComponentEndOverlap ( Collider* collider , Collider* other) override;

private:
	Flipbook* _flipbookUp = nullptr;
	Flipbook* _flipbookDown = nullptr;
	Flipbook* _flipbookLeft = nullptr;
	Flipbook* _flipbookRight = nullptr;
};

