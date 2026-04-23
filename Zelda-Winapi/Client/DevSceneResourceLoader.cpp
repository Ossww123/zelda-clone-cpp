#include "pch.h"
#include "DevSceneResourceLoader.h"

#include "ResourceManager.h"
#include "Texture.h"
#include "Flipbook.h"
#include "SoundManager.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <array>

namespace
{
	string ResolveExistingPath ( const string& relativePath )
	{
		const array<string , 4> candidates =
		{
			relativePath ,
			"../" + relativePath ,
			"../../" + relativePath ,
			"../../../" + relativePath
		};

		for ( const string& candidate : candidates )
		{
			std::error_code ec;
			if ( std::filesystem::exists ( std::filesystem::path ( candidate ) , ec ) )
				return candidate;
		}

		return relativePath;
	}

	string ReadAllText ( const string& filePath )
	{
		ifstream ifs ( filePath );
		if ( !ifs.is_open ( ) )
			return {};

		ostringstream oss;
		oss << ifs.rdbuf ( );
		return oss.str ( );
	}

	wstring ToWideAscii ( const string& value )
	{
		return wstring ( value.begin ( ) , value.end ( ) );
	}

	size_t FindKey ( const string& json , const string& key , size_t startPos )
	{
		string searchKey = "\"" + key + "\"";
		return json.find ( searchKey , startPos );
	}

	string GetStringValue ( const string& json , const string& key , size_t startPos )
	{
		size_t keyPos = FindKey ( json , key , startPos );
		if ( keyPos == string::npos )
			return "";

		size_t colonPos = json.find ( ":" , keyPos );
		if ( colonPos == string::npos )
			return "";

		size_t quoteStart = json.find ( "\"" , colonPos );
		if ( quoteStart == string::npos )
			return "";

		size_t quoteEnd = json.find ( "\"" , quoteStart + 1 );
		if ( quoteEnd == string::npos )
			return "";

		return json.substr ( quoteStart + 1 , quoteEnd - quoteStart - 1 );
	}

	int32 GetIntValue ( const string& json , const string& key , size_t startPos )
	{
		size_t keyPos = FindKey ( json , key , startPos );
		if ( keyPos == string::npos )
			return 0;

		size_t colonPos = json.find ( ":" , keyPos );
		if ( colonPos == string::npos )
			return 0;

		size_t numStart = colonPos + 1;
		while ( numStart < json.length ( ) && ( json[numStart] == ' ' || json[numStart] == '\t' || json[numStart] == '\n' ) )
			numStart++;

		size_t numEnd = numStart;
		while ( numEnd < json.length ( ) && ( isdigit ( json[numEnd] ) || json[numEnd] == '-' ) )
			numEnd++;

		if ( numEnd > numStart )
		{
			string numStr = json.substr ( numStart , numEnd - numStart );
			return stoi ( numStr );
		}

		return 0;
	}

	vector<int32> GetIntArray ( const string& json , const string& key , size_t startPos )
	{
		vector<int32> result;

		size_t keyPos = ( key.empty ( ) ) ? startPos : FindKey ( json , key , startPos );
		if ( keyPos == string::npos )
			return result;

		size_t colonPos = ( key.empty ( ) ) ? keyPos : json.find ( ":" , keyPos );
		size_t arrayStart = json.find ( "[" , colonPos );
		if ( arrayStart == string::npos )
			return result;

		size_t arrayEnd = json.find ( "]" , arrayStart );
		if ( arrayEnd == string::npos )
			return result;

		size_t pos = arrayStart + 1;
		while ( pos < arrayEnd )
		{
			while ( pos < arrayEnd && ( json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == ',' ) )
				pos++;

			if ( pos >= arrayEnd )
				break;

			size_t numEnd = pos;
			while ( numEnd < arrayEnd && ( isdigit ( json[numEnd] ) || json[numEnd] == '-' ) )
				numEnd++;

			if ( numEnd > pos )
			{
				string numStr = json.substr ( pos , numEnd - pos );
				result.push_back ( stoi ( numStr ) );
				pos = numEnd;
			}
			else
			{
				pos++;
			}
		}

		return result;
	}

	string GetObjectBlock ( const string& json , size_t startPos , size_t& outEndPos )
	{
		size_t objStart = json.find ( "{" , startPos );
		if ( objStart == string::npos )
			return "";

		int braceCount = 1;
		size_t pos = objStart + 1;

		while ( pos < json.length ( ) && braceCount > 0 )
		{
			if ( json[pos] == '{' )
				braceCount++;
			else if ( json[pos] == '}' )
				braceCount--;

			pos++;
		}

		if ( braceCount == 0 )
		{
			outEndPos = pos;
			return json.substr ( objStart , pos - objStart );
		}

		return "";
	}

	vector<string> GetObjectsInArray ( const string& json , const string& arrayKey , size_t startPos )
	{
		vector<string> result;

		size_t keyPos = FindKey ( json , arrayKey , startPos );
		if ( keyPos == string::npos )
			return result;

		size_t colonPos = json.find ( ":" , keyPos );
		if ( colonPos == string::npos )
			return result;

		size_t arrayStart = json.find ( "[" , colonPos );
		if ( arrayStart == string::npos )
			return result;

		int bracketCount = 1;
		size_t arrayEnd = arrayStart + 1;
		while ( arrayEnd < json.length ( ) && bracketCount > 0 )
		{
			if ( json[arrayEnd] == '[' )
				bracketCount++;
			else if ( json[arrayEnd] == ']' )
				bracketCount--;
			arrayEnd++;
		}

		size_t pos = arrayStart + 1;
		while ( pos < arrayEnd )
		{
			while ( pos < arrayEnd && ( json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == ',' ) )
				pos++;

			if ( pos >= arrayEnd || json[pos] == ']' )
				break;

			size_t endPos = 0;
			string objBlock = GetObjectBlock ( json , pos , endPos );
			if ( objBlock.empty ( ) )
				break;

			result.push_back ( objBlock );
			pos = endPos;
		}

		return result;
	}

	bool ParseBoolValue ( const string& json , const string& key , size_t startPos , bool defaultValue = false )
	{
		size_t keyPos = FindKey ( json , key , startPos );
		if ( keyPos == string::npos )
			return defaultValue;

		size_t colonPos = json.find ( ":" , keyPos );
		if ( colonPos == string::npos )
			return defaultValue;

		size_t valueStart = colonPos + 1;
		while ( valueStart < json.length ( ) && ( json[valueStart] == ' ' || json[valueStart] == '\t' || json[valueStart] == '\n' ) )
			valueStart++;

		if ( json.compare ( valueStart , 4 , "true" ) == 0 )
			return true;
		if ( json.compare ( valueStart , 5 , "false" ) == 0 )
			return false;

		return defaultValue;
	}

	bool TryGetColorKeyField ( const string& objectText , int32& r , int32& g , int32& b )
	{
		vector<int32> values = GetIntArray ( objectText , "colorkey" , 0 );
		if ( values.size ( ) < 3 )
			return false;

		r = values[0];
		g = values[1];
		b = values[2];
		return true;
	}

	string ExtractFilePathValue ( const string& objectText )
	{
		string value = GetStringValue ( objectText , "path" , 0 );
		if ( value.empty ( ) )
			return value;

		for ( char& c : value )
		{
			if ( c == '/' )
				c = '\\';
		}

		return value;
	}
}

void DevSceneResourceLoader::LoadSceneTextures ( )
{
	string jsonPath = ResolveExistingPath ( "../DatasheetsClient/Textures.json" );
	string json = ReadAllText ( jsonPath );
	if ( json.empty ( ) )
		return;

	vector<string> objects = GetObjectsInArray ( json , "textures" , 0 );

	for ( const string& obj : objects )
	{
		string id = GetStringValue ( obj , "id" , 0 );
		if ( id.empty ( ) )
			continue;
		string path = ExtractFilePathValue ( obj );
		if ( path.empty ( ) )
			continue;

		wstring wid = ToWideAscii ( id );

		int32 r = 0;
		int32 g = 0;
		int32 b = 0;
		if ( TryGetColorKeyField ( obj , r , g , b ) )
			GET_SINGLE ( ResourceManager )->LoadTexture ( wid , ToWideAscii ( path ) , RGB ( r , g , b ) );
		else
			GET_SINGLE ( ResourceManager )->LoadTexture ( wid , ToWideAscii ( path ) );
	}
}

void DevSceneResourceLoader::CreateSceneSprites ( )
{
	string jsonPath = ResolveExistingPath ( "../DatasheetsClient/Sprites.json" );
	string json = ReadAllText ( jsonPath );
	if ( json.empty ( ) )
		return;

	vector<string> objects = GetObjectsInArray ( json , "sprites" , 0 );

	for ( const string& obj : objects )
	{
		string id = GetStringValue ( obj , "id" , 0 );
		if ( id.empty ( ) )
			continue;
		string textureId = GetStringValue ( obj , "texture" , 0 );
		if ( textureId.empty ( ) )
			continue;
		int32 x = GetIntValue ( obj , "x" , 0 );
		int32 y = GetIntValue ( obj , "y" , 0 );
		int32 w = GetIntValue ( obj , "w" , 0 );
		int32 h = GetIntValue ( obj , "h" , 0 );

		wstring wid = ToWideAscii ( id );
		wstring wTextureId = ToWideAscii ( textureId );

		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( wTextureId );
		if ( texture == nullptr )
			continue;

		GET_SINGLE ( ResourceManager )->CreateSprite ( wid , texture , x , y , w , h );
	}
}

void DevSceneResourceLoader::LoadFlipbooks ( )
{
	// Player - IDLE
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );    auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleUp" );    fb->SetInfo ( { t, L"FB_IdleUp",    {200, 200}, 0, 9, 0, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleDown" );  fb->SetInfo ( { t, L"FB_IdleDown",  {200, 200}, 0, 9, 0, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleLeft" );  fb->SetInfo ( { t, L"FB_IdleLeft",  {200, 200}, 0, 9, 0, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" ); auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleRight" ); fb->SetInfo ( { t, L"FB_IdleRight", {200, 200}, 0, 9, 0, 0.5f } ); }

	// Player - MOVE
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );    auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveUp" );    fb->SetInfo ( { t, L"FB_MoveUp",    {200, 200}, 0, 9, 1, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveDown" );  fb->SetInfo ( { t, L"FB_MoveDown",  {200, 200}, 0, 9, 1, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveLeft" );  fb->SetInfo ( { t, L"FB_MoveLeft",  {200, 200}, 0, 9, 1, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" ); auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveRight" ); fb->SetInfo ( { t, L"FB_MoveRight", {200, 200}, 0, 9, 1, 0.5f } ); }

	// Player - ATTACK (Sword)
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );    auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackUp" );    fb->SetInfo ( { t, L"FB_AttackUp",    {200, 200}, 0, 7, 3, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackDown" );  fb->SetInfo ( { t, L"FB_AttackDown",  {200, 200}, 0, 7, 3, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackLeft" );  fb->SetInfo ( { t, L"FB_AttackLeft",  {200, 200}, 0, 7, 3, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" ); auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackRight" ); fb->SetInfo ( { t, L"FB_AttackRight", {200, 200}, 0, 7, 3, 0.5f, false } ); }

	// Player - BOW
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );    auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowUp" );    fb->SetInfo ( { t, L"FB_BowUp",    {200, 200}, 0, 7, 5, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowDown" );  fb->SetInfo ( { t, L"FB_BowDown",  {200, 200}, 0, 7, 5, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowLeft" );  fb->SetInfo ( { t, L"FB_BowLeft",  {200, 200}, 0, 7, 5, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" ); auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowRight" ); fb->SetInfo ( { t, L"FB_BowRight", {200, 200}, 0, 7, 5, 0.5f, false } ); }

	// Player - STAFF
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );    auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffUp" );    fb->SetInfo ( { t, L"FB_StaffUp",    {200, 200}, 0, 10, 6, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffDown" );  fb->SetInfo ( { t, L"FB_StaffDown",  {200, 200}, 0, 10, 6, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );  auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffLeft" );  fb->SetInfo ( { t, L"FB_StaffLeft",  {200, 200}, 0, 10, 6, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" ); auto* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffRight" ); fb->SetInfo ( { t, L"FB_StaffRight", {200, 200}, 0, 10, 6, 0.5f, false } ); }

	// Snake
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeUp" )   ->SetInfo ( { t, L"FB_SnakeUp",    {100, 100}, 0, 3, 3, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeDown" ) ->SetInfo ( { t, L"FB_SnakeDown",  {100, 100}, 0, 3, 0, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeLeft" ) ->SetInfo ( { t, L"FB_SnakeLeft",  {100, 100}, 0, 3, 2, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeRight" )->SetInfo ( { t, L"FB_SnakeRight", {100, 100}, 0, 3, 1, 0.5f } ); }

	// Octoroc
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocUp" )   ->SetInfo ( { t, L"FB_OctorocUp",    {100, 100}, 0, 3, 3, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocDown" ) ->SetInfo ( { t, L"FB_OctorocDown",  {100, 100}, 0, 3, 0, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocLeft" ) ->SetInfo ( { t, L"FB_OctorocLeft",  {100, 100}, 0, 3, 2, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocRight" )->SetInfo ( { t, L"FB_OctorocRight", {100, 100}, 0, 3, 1, 0.5f } ); }

	// Arrow
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowUp" )   ->SetInfo ( { t, L"FB_ArrowUp",    {100, 100}, 0, 0, 3, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowDown" ) ->SetInfo ( { t, L"FB_ArrowDown",  {100, 100}, 0, 0, 0, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowLeft" ) ->SetInfo ( { t, L"FB_ArrowLeft",  {100, 100}, 0, 0, 1, 0.5f } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowRight" )->SetInfo ( { t, L"FB_ArrowRight", {100, 100}, 0, 0, 2, 0.5f } ); }

	// Effects
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Hit" );     GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_Hit" )    ->SetInfo ( { t, L"FB_Hit",     {50,  47},  0, 5,  0, 0.5f, false } ); }
	{ Texture* t = GET_SINGLE ( ResourceManager )->GetTexture ( L"Explode" ); GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_Explode" )->SetInfo ( { t, L"FB_Explode", {144, 144}, 0, 11, 0, 0.5f, false } ); }
}

void DevSceneResourceLoader::LoadSceneSounds ( )
{
	string jsonPath = ResolveExistingPath ( "../DatasheetsClient/Sounds.json" );
	string json = ReadAllText ( jsonPath );
	if ( json.empty ( ) )
		return;

	vector<string> objects = GetObjectsInArray ( json , "sounds" , 0 );

	for ( const string& obj : objects )
	{
		string id = GetStringValue ( obj , "id" , 0 );
		if ( id.empty ( ) )
			continue;
		string path = ExtractFilePathValue ( obj );
		if ( path.empty ( ) )
			continue;

		wstring wid = ToWideAscii ( id );
		wstring wpath = ToWideAscii ( path );

		GET_SINGLE ( ResourceManager )->LoadSound ( wid , wpath );

		bool loopOnInit = ParseBoolValue ( obj , "loop_on_init" , 0 , false );
		if ( loopOnInit )
			GET_SINGLE ( SoundManager )->Play ( wid , true );
	}
}

