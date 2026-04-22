#include "pch.h"
#include "Panel.h"
#include "InputManager.h"

Panel::Panel ( )
{
}

Panel::~Panel ( )
{
	for ( ChildEntry& child : _children )
		SAFE_DELETE ( child.ui );

	_children.clear ( );
}

void Panel::BeginPlay ( )
{
	if ( IsVisible ( ) == false )
		return;

	Super::BeginPlay ( );
	SyncChildren ( );

	for ( ChildEntry& child : _children )
	{
		if ( child.ui )
			child.ui->BeginPlay ( );
	}
}

void Panel::Tick ( )
{
	if ( IsVisible ( ) == false )
		return;

	Super::Tick ( );

	if ( IsEnabled ( ) && _draggable )
	{
		POINT mousePos = GET_SINGLE ( InputManager )->GetMousePos ( );

		if ( _dragging )
		{
			bool held = GET_SINGLE ( InputManager )->GetButton ( KeyType::LeftMouse )
				|| GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::LeftMouse );

			if ( held )
			{
				RECT rect = GetRect ( );
				int32 panelW = rect.right - rect.left;
				int32 panelH = rect.bottom - rect.top;

				int32 left = mousePos.x - _dragOffset.x;
				int32 top = mousePos.y - _dragOffset.y;
				SetPos ( { ( float ) ( left + panelW / 2 ), ( float ) ( top + panelH / 2 ) } );
			}
			else
			{
				_dragging = false;
			}
		}
		else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::LeftMouse ) )
		{
			RECT rect = GetRect ( );
			RECT handleRect = rect;
			handleRect.bottom = min ( handleRect.bottom , handleRect.top + _dragHandleHeight );

			if ( ::PtInRect ( &handleRect , mousePos ) )
			{
				_dragging = true;
				_dragOffset = { mousePos.x - rect.left, mousePos.y - rect.top };
			}
		}
	}

	SyncChildren ( );

	for ( ChildEntry& child : _children )
	{
		if ( child.ui && child.ui->IsVisible ( ) )
			child.ui->Tick ( );
	}
}

void Panel::Render ( HDC hdc )
{
	if ( IsVisible ( ) == false )
		return;

	Super::Render ( hdc );

	for ( ChildEntry& child : _children )
	{
		if ( child.ui && child.ui->IsVisible ( ) )
			child.ui->Render ( hdc );
	}
}

void Panel::AddChild ( UI* ui )
{
	if ( ui == nullptr )
		return;

	Vec2 panelPos = GetPos ( );
	Vec2 childPos = ui->GetPos ( );
	Vec2Int localOffset =
	{
		( int32 ) ( childPos.x - panelPos.x ),
		( int32 ) ( childPos.y - panelPos.y )
	};

	AddChild ( ui , localOffset );
}

void Panel::AddChild ( UI* ui , Vec2Int localOffset )
{
	if ( ui == nullptr )
		return;

	ChildEntry entry;
	entry.ui = ui;
	entry.localOffset = localOffset;
	_children.push_back ( entry );

	SyncChildren ( );
}

bool Panel::RemoveChild ( UI* ui )
{
	for ( auto it = _children.begin ( ); it != _children.end ( ); ++it )
	{
		if ( it->ui != ui )
			continue;

		_children.erase ( it );
		return true;
	}

	return false;
}

void Panel::SetChildLocalOffset ( UI* ui , Vec2Int localOffset )
{
	if ( ui == nullptr )
		return;

	for ( ChildEntry& child : _children )
	{
		if ( child.ui != ui )
			continue;

		child.localOffset = localOffset;
		SyncChildren ( );
		return;
	}
}

void Panel::SyncChildren ( )
{
	Vec2 panelPos = GetPos ( );

	for ( ChildEntry& child : _children )
	{
		if ( child.ui == nullptr )
			continue;

		Vec2 pos =
		{
			panelPos.x + child.localOffset.x,
			panelPos.y + child.localOffset.y
		};
		child.ui->SetPos ( pos );
	}
}
