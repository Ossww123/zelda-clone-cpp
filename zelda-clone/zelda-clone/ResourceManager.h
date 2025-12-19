#pragma once

class ResourceBase;

class ResourceManager
{
	DECLARE_SINGLE ( ResourceManager );
public:
	~ResourceManager ( );

public:
	void Init (HWND hwnd, fs::path resourcePath );
	void Clear ( );

	const fs::path& GetResourcePath ( ) { return _resourcePath; }

private:
	HWND _hwnd;
	fs::path _resourcePath;
};

