#include "pch.h"
#include "Effect.h"
#include "SceneManager.h"
#include "Scene.h"

Effect::Effect ( )
{
	SetLayer ( LAYER_EFFECT );
}

void Effect::BeginPlay ( )
{
	Super::BeginPlay ( );
	UpdateAnimation ( );
}

void Effect::Tick ( )
{
	Super::Tick ( );

	if ( IsAnimationEnded ( ) )
	{
		RemoveFromScene ( );
	}
}

void Effect::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void Effect::RemoveFromScene ( )
{
	Scene* scene = GET_SINGLE ( SceneManager )->GetCurrentScene ( );
	if ( scene )
		scene->RemoveActor ( this );
}
