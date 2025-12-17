#include "pch.h"
#include "ObjectManager.h"

ObjectManager::~ObjectManager ( )
{
	Clear ( );
}

void ObjectManager::Add ( Object* object )
{
	if ( object == nullptr )
		return;

	auto findIt = std::find ( _objects.begin ( ) , _objects.end ( ) , object );
	if ( findIt != _objects.end ( ) )
		return;

	_objects.push_back ( object );
}

void ObjectManager::Remove ( Object* object )
{
	if ( object == nullptr )
		return;

	auto it = std::remove ( _objects.begin ( ) , _objects.end ( ) , object );
	_objects.erase ( it , _objects.end ( ) );

	delete object;
}

void ObjectManager::Clear ( )
{
	std::for_each ( _objects.begin ( ) , _objects.end ( ) , [ = ] ( Object* obj ) {delete obj; } );
	
	_objects.clear ( );
}
