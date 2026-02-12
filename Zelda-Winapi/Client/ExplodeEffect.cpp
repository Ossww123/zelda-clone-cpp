#include "pch.h"
#include "ExplodeEffect.h"
#include "ResourceManager.h"

ExplodeEffect::ExplodeEffect ( )
{
	SetLayer ( LAYER_EFFECT );
}

ExplodeEffect::~ExplodeEffect ( )
{
}

void ExplodeEffect::BeginPlay ( )
{
	Super::BeginPlay ( );
}

void ExplodeEffect::Tick ( )
{
	Super::Tick ( );
}

void ExplodeEffect::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void ExplodeEffect::UpdateAnimation ( )
{
	SetFlipbook ( GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_Explode" ) );
}
