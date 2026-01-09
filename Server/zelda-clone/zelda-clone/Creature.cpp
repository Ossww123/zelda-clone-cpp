#include "pch.h"
#include "Creature.h"
#include "Scene.h"
#include "DevScene.h"
#include "SceneManager.h"

Creature::Creature ( )
{
}

Creature::~Creature ( )
{
}

void Creature::BeginPlay ( )
{
	Super::BeginPlay ( );

	// TODO
}

void Creature::Tick ( )
{
	Super::Tick ( );

	// TODO
}

void Creature::Render ( HDC hdc )
{
	Super::Render ( hdc );

	// TODO
}

void Creature::OnDamaged ( Creature* attacker )
{
	if ( attacker == nullptr )
		return;

	Status& attackerStatus = attacker->GetStatus ( );
	Status& status = GetStatus ( );

	int32 damage = attackerStatus.attack - status.defence;
	if ( damage <= 0 )
		return;

	status.hp = max ( 0 , status.hp - damage );

	if ( status.hp == 0 )
	{
		Scene* scene = GET_SINGLE ( SceneManager )->GetCurrentScene ( );
		if ( scene )
		{
			scene->RemoveActor ( this );
		}
	}
}
