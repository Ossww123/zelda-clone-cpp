#pragma once
#include "Panel.h"

class Sprite;

class InventoryPanel : public Panel
{
	using Super = Panel;

public:
	InventoryPanel ( );
	virtual ~InventoryPanel ( ) override;

	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:
	bool PrepareLayout ( );
	void HandleInventoryClick ( );
	Sprite* GetItemSprite ( int32 itemId );
};

