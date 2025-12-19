#include "pch.h"
#include "ResourceManager.h"

ResourceManager::~ResourceManager ( )
{
	Clear ( );
}

void ResourceManager::Init ( HWND hwnd , fs::path resourcePath )
{
	_hwnd = hwnd;
	_resourcePath = resourcePath;

}

void ResourceManager::Clear ( )
{

}
