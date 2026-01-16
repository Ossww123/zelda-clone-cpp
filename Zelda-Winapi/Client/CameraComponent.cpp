#include "pch.h"
#include "CameraComponent.h"

#include "Actor.h"

#include "DevScene.h"
#include "SceneManager.h"

CameraComponent::CameraComponent ( )
{
}

CameraComponent::~CameraComponent ( )
{
}

void CameraComponent::BeginPlay ( )
{
}

void CameraComponent::TickComponent ( )
{
	if ( _owner == nullptr )
		return;

	Vec2 pos = _owner->GetPos ( );

	DevScene* scene = dynamic_cast< DevScene* >( GET_SINGLE ( SceneManager )->GetCurrentScene ( ) );
	if ( scene == nullptr )
		return;

	Vec2Int worldSize = scene->GetWorldPixelSize ( );
	if ( worldSize.x <= 0 || worldSize.y <= 0 )
		return;

	const float halfW = ( float ) GWinSizeX * 0.5f;
	const float halfH = ( float ) GWinSizeY * 0.5f;

	float minX = halfW;
	float maxX = ( float ) worldSize.x - halfW;
	float minY = halfH;
	float maxY = ( float ) worldSize.y - halfH;

	if ( maxX < minX ) maxX = minX;
	if ( maxY < minY ) maxY = minY;

	pos.x = ::clamp ( pos.x , minX , maxX );
	pos.y = ::clamp ( pos.y , minY , maxY );

	GET_SINGLE ( SceneManager )->SetCameraPos ( pos );
}

void CameraComponent::Render ( HDC hdc )
{
}
