#pragma once

class Actor;
class UI;

class Scene
{
public:
	Scene ( );
	virtual ~Scene ( );

	virtual void Init ( );
	virtual void Update ( );
	virtual void Render ( HDC hdc );

public:
	void AddActor ( Actor* actor );
	void RemoveActor ( Actor* actor );

public:
	vector<Actor*> _actors[ LAYER_MAXCOUNT ];
	vector<UI*> _uis;

};

