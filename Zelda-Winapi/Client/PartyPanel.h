#pragma once
#include "Panel.h"

class PartyPanel : public Panel
{
	using Super = Panel;

public:
	PartyPanel ( );
	virtual ~PartyPanel ( ) override;

	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:
	bool PrepareLayout ( );
};

