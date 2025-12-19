#pragma once
class Component
{
public:
	virtual void Start ( ) {}
	virtual void Update ( ) {}
	virtual void Render ( HDC hdc ) {}

	// owner
	// GameObject* _owner;
};

