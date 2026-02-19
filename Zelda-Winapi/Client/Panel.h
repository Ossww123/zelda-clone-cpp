#pragma once
#include "UI.h"

class Sprite;

class Panel : public UI
{
	using Super = UI;
public:
	Panel ( );
	virtual ~Panel ( );

	virtual void BeginPlay ( ) override;
	virtual void Tick ( ) override;
	virtual void Render ( HDC hdc ) override;

	void AddChild ( UI* ui );
	void AddChild ( UI* ui , Vec2Int localOffset );
	bool RemoveChild ( UI* ui );

	void SetDraggable ( bool draggable ) { _draggable = draggable; }
	bool IsDraggable ( ) const { return _draggable; }
	bool IsDragging ( ) const { return _dragging; }
	void SetDragHandleHeight ( int32 height ) { _dragHandleHeight = ( height < 0 ? 0 : height ); }
	int32 GetDragHandleHeight ( ) const { return _dragHandleHeight; }

	void SetChildLocalOffset ( UI* ui , Vec2Int localOffset );

private:
	struct ChildEntry
	{
		UI* ui = nullptr;
		Vec2Int localOffset = {};
	};

	void SyncChildren ( );

private:
	vector<ChildEntry> _children;
	bool _draggable = false;
	bool _dragging = false;
	int32 _dragHandleHeight = 30;
	Vec2Int _dragOffset = {};
};

