#pragma once
#include "ResourceBase.h"

class LineMesh : public ResourceBase
{
public:
	void Save ( wstring path );
	void Load ( wstring path );

	void Render ( HDC hdc , Pos pos ) const;

protected:
	vector<pair<POINT , POINT>> _lines;
};

