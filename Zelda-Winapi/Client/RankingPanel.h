#pragma once
#include "Panel.h"

class RankingPanel : public Panel
{
	using Super = Panel;

public:
	RankingPanel ( );
	virtual ~RankingPanel ( ) override;

	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

	void SetData ( const vector<pair<wstring, int32>>& entries );

private:
	vector<pair<wstring, int32>> _entries;
};

