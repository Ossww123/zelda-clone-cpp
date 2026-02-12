#include "pch.h"
#include "HitEffect.h"
#include "ResourceManager.h"

HitEffect::HitEffect ( )
{
	SetLayer ( LAYER_EFFECT );
}

HitEffect::~HitEffect ( )
{
}

void HitEffect::BeginPlay ( )
{
	Super::BeginPlay ( );
}

void HitEffect::Tick ( )
{
	Super::Tick ( );
}

void HitEffect::Render ( HDC hdc )
{
	Super::Render ( hdc );
}

void HitEffect::UpdateAnimation ( )
{
	SetFlipbook(GET_SINGLE ( ResourceManager )->GetFlipbook ( L"FB_Hit" ));
}
