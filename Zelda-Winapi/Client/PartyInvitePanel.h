#pragma once
#include "Panel.h"

class PartyInvitePanel : public Panel
{
	using Super = Panel;

public:
	PartyInvitePanel ( );
	virtual ~PartyInvitePanel ( ) override;

	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

private:
	bool PrepareLayout ( int32& outW , int32& outH ) const;
};

