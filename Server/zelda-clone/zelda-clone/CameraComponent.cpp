#include "pch.h"
#include "CameraComponent.h"

#include "Actor.h"

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

	// TEMP
	pos.x = ::clamp ( pos.x , 400.f , 2560.f - 400.f );
	pos.y = ::clamp ( pos.y , 300.f , 2048.f - 400.f );

	GET_SINGLE ( SceneManager )->SetCameraPos ( pos );
}

void CameraComponent::Render ( HDC hdc )
{
}
