#include "pch.h"
#include "DevScene.h"

#include "Utils.h"
#include "Texture.h"
#include "Sprite.h"
#include "Actor.h"
#include "SpriteActor.h"
#include "Flipbook.h"
#include "Player.h"
#include "Arrow.h"
#include "UI.h"
#include "Button.h"
#include "Tilemap.h"
#include "TilemapActor.h"
#include "Sound.h"
#include "Monster.h"

#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "NetworkManager.h"
#include "ClientPacketHandler.h"
#include "InventoryPanel.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <array>

// JSON 파싱 유틸 함수들
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

DevScene::DevScene ( )
{
}

DevScene::~DevScene ( )
{
	SAFE_DELETE ( _inventoryPanel );
}

void DevScene::Init ( )
{
	LoadSceneTextures ( );
	CreateSceneSprites ( );
	LoadMap ( );
	LoadPlayer ( );
	LoadMonster ( );
	LoadProjectiles ( );
	LoadEffect ( );
	LoadTilemap ( );

	_currentMapId = Protocol::MAP_ID_TOWN;
	_hasMapId = true;

	LoadSceneSounds ( );
	

	_inventoryPanel = new InventoryPanel ( );

	CreateMapButtons ( );
	Super::Init ( );
}

void DevScene::Update ( )
{
	if ( !_loggedIn )
	{
		UpdateLogin ( );
		return;
	}

	Super::Update ( );

	float deltaTime = GET_SINGLE ( TimeManager )->GetDeltaTime ( );

	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::I ) )
	{
		if ( _inventoryPanel )
			_inventoryPanel->SetVisible ( !_inventoryPanel->IsVisible ( ) );
		GET_SINGLE ( SoundManager )->Play ( L"UISound" );
	}
	if ( _inventoryPanel )
		_inventoryPanel->Tick ( );
	HandlePartyInput ( );
}

void DevScene::Render ( HDC hdc )
{
	if ( !_loggedIn )
	{
		RenderLogin ( hdc );
		return;
	}

	Super::Render ( hdc );
	RenderHUD ( hdc );
	RenderPartyHUD ( hdc );
	RenderPartyInvite ( hdc );

	if ( _inventoryPanel )
		_inventoryPanel->Render ( hdc );
}

void DevScene::AddActor ( Actor* actor )
{
	Super::AddActor ( actor );
}

void DevScene::RemoveActor ( Actor* actor )
{
	Super::RemoveActor ( actor );
}

void DevScene::ChangeMap ( Protocol::MAP_ID mapId )
{
	_currentMapId = mapId;
	_hasMapId = true;

	ClearWorldActors ( );
	ChangeBackground ( mapId );

	// 타일맵 교체
	if ( mapId == Protocol::MAP_ID_TOWN )
	{
		LoadTilemap ( L"Tilemap\\Tilemap_01.txt" );
	}
	else if ( mapId == Protocol::MAP_ID_DUNGEON )
	{
		LoadTilemap ( L"Tilemap\\Tilemap_02.txt" );
	}
}

void DevScene::ChangeBackground ( Protocol::MAP_ID mapId )
{
	if ( _background == nullptr )
		return;

	Sprite* sprite = nullptr;

	if ( mapId == Protocol::MAP_ID_TOWN )
		sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage01" );
	else if ( mapId == Protocol::MAP_ID_DUNGEON )
		sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage02" );

	if ( sprite == nullptr )
		return;

	_background->SetSprite ( sprite );

	const Vec2Int size = sprite->GetSize ( );
	_background->SetPos ( Vec2 ( size.x / 2 , size.y / 2 ) );
}

void DevScene::Handle_S_AddObject ( Protocol::S_AddObject& pkt )
{
	uint64 myPlayerId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );

	const int32 size = pkt.objects_size ( );
	for ( int32 i = 0; i < size; i++ )
	{
		const Protocol::ObjectInfo& info = pkt.objects ( i );
		if ( myPlayerId == info.objectid ( ) )
			continue;

		if ( info.objecttype ( ) == Protocol::OBJECT_TYPE_PLAYER )
		{
			Player* player = SpawnObject<Player> ( Vec2Int{ info.posx ( ), info.posy ( ) } );
			player->SetDir ( info.dir ( ) );
			player->SetState ( info.state ( ) );
			player->info = info;
		}
		else if ( info.objecttype ( ) == Protocol::OBJECT_TYPE_MONSTER )
		{
			Monster* monster = SpawnObject<Monster> ( Vec2Int{ info.posx ( ), info.posy ( ) } );
			monster->info = info;
			monster->SetDir ( info.dir ( ) );
			monster->SetState ( info.state ( ) );
			monster->SetCellPos ( Vec2Int{ info.posx ( ), info.posy ( ) } , true );
		}
		else if ( info.objecttype() == Protocol::OBJECT_TYPE_PROJECTILE)
		{
			 Arrow* arrow = SpawnObject<Arrow>( Vec2Int{info.posx(), info.posy()});
			 arrow->info = info;
			 arrow->SetDir(info.dir());
			 arrow->SetState(info.state());
			 arrow->SetCellPos({info.posx(), info.posy()}, true);
		}
	}
}

void DevScene::Handle_S_RemoveObject ( Protocol::S_RemoveObject& pkt )
{
	const int32 size = pkt.ids_size ( );
	for ( int32 i = 0; i < size; i++ )
	{
		int32 id = pkt.ids ( i );

		GameObject* object =  GetObject( id );
		if ( object )
			RemoveActor ( object );
	}
}

GameObject* DevScene::GetObject ( uint64 id )
{
	for ( Actor* actor : _actors[ LAYER_OBJECT ] )
	{
		GameObject* gameObject = dynamic_cast< GameObject* >( actor );
		if ( gameObject && gameObject->info.objectid ( ) == id )
			return gameObject;
	}

	return nullptr;
}

Player* DevScene::FindClosestPlayer ( Vec2Int cellPos )
{
	float best = FLT_MAX;
	Player* ret = nullptr;

	for ( Actor* actor : _actors[ LAYER_OBJECT ] )
	{
		Player* player = dynamic_cast< Player* >( actor );
		if ( player )
		{
			Vec2Int dir = cellPos - player->GetCellPos ( );
			float dist = dir.LengthSquared ( );
			if ( dist < best )
			{
				dist = best;
				ret = player;
			}
		}
	}
	return ret;
}

bool DevScene::CanGo ( Vec2Int cellPos )
{
	if ( _tilemapActor == nullptr )
		return false;

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return false;

	Tile* tile = tm->GetTileAt ( cellPos );
	if ( tile == nullptr )
		return false;

	if ( GetCreatureAt ( cellPos ) != nullptr )
		return false;

	return tile->value != 1;
}

Vec2 DevScene::ConvertPos ( Vec2Int cellPos )
{
	Vec2 ret = {};

	if ( _tilemapActor == nullptr )
		return ret;

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return ret;

	int32 size = tm->GetTileSize ( );
	Vec2 pos = _tilemapActor->GetPos ( );

	ret.x = pos.x + cellPos.x * size + ( size / 2 );
	ret.y = pos.y + cellPos.y * size + ( size / 2 );

	return ret;
}

Vec2Int DevScene::GetWorldPixelSize ( ) const
{
	if ( _tilemapActor == nullptr )
		return { 0, 0 };

	Tilemap* tm = _tilemapActor->GetTilemap ( );
	if ( tm == nullptr )
		return { 0, 0 };

	Vec2Int mapSize = tm->GetMapSize ( );
	int32 tileSize = tm->GetTileSize ( );

	return { mapSize.x * tileSize, mapSize.y * tileSize };
}


void DevScene::UpdateLogin ( )
{
	// A-Z
	for ( int32 vk = 'A'; vk <= 'Z'; vk++ )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( vk ) ) )
		{
			if ( ( int32 ) _loginText.length ( ) < MAX_USERNAME_LEN )
				_loginText += static_cast< wchar_t >( vk - 'A' + L'a' );
		}
	}

	// 0-9
	for ( int32 vk = '0'; vk <= '9'; vk++ )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( vk ) ) )
		{
			if ( ( int32 ) _loginText.length ( ) < MAX_USERNAME_LEN )
				_loginText += static_cast< wchar_t >( vk );
		}
	}

	// Backspace
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_BACK ) ) )
	{
		if ( !_loginText.empty ( ) )
			_loginText.pop_back ( );
	}

	// Enter
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( static_cast< KeyType >( VK_RETURN ) ) )
	{
		if ( !_loginText.empty ( ) )
		{
			// wstring -> string
			string username ( _loginText.begin ( ) , _loginText.end ( ) );
			SendBufferRef sendBuffer = ClientPacketHandler::Make_C_Login ( username );
			GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
		}
	}
}

void DevScene::RenderLogin ( HDC hdc )
{
	Sprite* panel = GET_SINGLE ( ResourceManager )->GetSprite ( L"LoginPanel" );
	if ( panel )
	{
		// 창 크기에 맞춰 스케일링
		::TransparentBlt (
			hdc ,
			0 , 0 ,
			GWinSizeX , GWinSizeY ,
			panel->GetDC ( ) , 
			panel->GetPos ( ).x , panel->GetPos ().y ,
			panel->GetSize ( ).x , panel->GetSize ( ).y ,
			RGB ( 0 , 0 , 0 ) // colorkey
		);
	}

	SetBkMode ( hdc , TRANSPARENT );

	int32 boxW = 300;
	int32 boxH = 150;
	int32 boxX = ( GWinSizeX - boxW ) / 2;
	int32 boxY = ( GWinSizeY - boxH ) / 2;

	// 입력 텍스트 + 커서
	SetTextColor ( hdc , RGB ( 0 , 0 , 0 ) );
	int32 fieldX = boxX + 30;
	int32 fieldY = boxY + 65;
	int32 fieldW = boxW - 60;
	int32 fieldH = 28;

	std::wstring displayText = _loginText + L"|";
	RECT textRect = { fieldX + 6, fieldY + 4, fieldX + fieldW - 6, fieldY + fieldH - 4 };
	DrawText (
		hdc ,
		displayText.c_str ( ) ,
		( int32 ) displayText.length ( ) ,
		&textRect ,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE
	);

	// 안내 문구
	SetTextColor ( hdc , RGB ( 180 , 180 , 180 ) );
	RECT hintRect = { boxX, boxY + 105, boxX + boxW, boxY + 130 };
	DrawText ( hdc , L"Enter your name and press Enter" , 31 , &hintRect , DT_CENTER );
}

void DevScene::RenderHUD ( HDC hdc )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	const int32 baseX = 36;
	const int32 baseY = 36;

	// Status Frame
	Sprite* frame = GET_SINGLE ( ResourceManager )->GetSprite ( L"Status_Frame" );
	if ( frame )
	{
		::TransparentBlt ( hdc ,
			baseX , baseY ,
			frame->GetSize ( ).x , frame->GetSize ( ).y ,
			frame->GetDC ( ) ,
			frame->GetPos ( ).x , frame->GetPos ( ).y ,
			frame->GetSize ( ).x , frame->GetSize ( ).y ,
			frame->GetTransparent ( ) );
	}

	// Weapon Icon (선택된 무기만 표시)
	{
		Sprite* weaponSprite = nullptr;
		switch ( myPlayer->GetWeaponType ( ) )
		{
		case WeaponType::Sword:
			weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Sword_Icon" );
			break;
		case WeaponType::Bow:
			weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Bow_Icon" );
			break;
		case WeaponType::Staff:
			weaponSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Staff_Icon" );
			break;
		}

		if ( weaponSprite )
		{
			::TransparentBlt ( hdc ,
				baseX + 0 , baseY + 0 ,
				weaponSprite->GetSize ( ).x , weaponSprite->GetSize ( ).y ,
				weaponSprite->GetDC ( ) ,
				weaponSprite->GetPos ( ).x , weaponSprite->GetPos ( ).y ,
				weaponSprite->GetSize ( ).x , weaponSprite->GetSize ( ).y ,
				weaponSprite->GetTransparent ( ) );
		}
	}

	// HP Bar
	{
		Sprite* hpBar = GET_SINGLE ( ResourceManager )->GetSprite ( L"Hp_Bar" );
		if ( hpBar )
		{
			int32 hp = myPlayer->info.hp ( );
			int32 maxHp = myPlayer->info.maxhp ( );
			int32 fullWidth = hpBar->GetSize ( ).x;
			int32 barWidth = ( maxHp > 0 ) ? ( fullWidth * hp / maxHp ) : 0;

			if ( barWidth > 0 )
			{
				::TransparentBlt ( hdc ,
					baseX + 99 , baseY + 9 ,
					barWidth , hpBar->GetSize ( ).y ,
					hpBar->GetDC ( ) ,
					hpBar->GetPos ( ).x , hpBar->GetPos ( ).y ,
					barWidth , hpBar->GetSize ( ).y ,
					hpBar->GetTransparent ( ) );
			}
		}
	}

	// EXP Bar
	{
		Sprite* expBar = GET_SINGLE ( ResourceManager )->GetSprite ( L"Exp_Bar" );
		if ( expBar )
		{
			const Protocol::PlayerExtra& extra = myPlayer->info.player ( );
			int32 exp = extra.exp ( );
			int32 maxExp = extra.maxexp ( );
			int32 fullWidth = expBar->GetSize ( ).x;
			int32 barWidth = ( maxExp > 0 ) ? ( fullWidth * exp / maxExp ) : 0;

			if ( barWidth > 0 )
			{
				::TransparentBlt ( hdc ,
					baseX + 9 , baseY + 36 ,
					barWidth , expBar->GetSize ( ).y ,
					expBar->GetDC ( ) ,
					expBar->GetPos ( ).x , expBar->GetPos ( ).y ,
					barWidth , expBar->GetSize ( ).y ,
					expBar->GetTransparent ( ) );
			}
		}
	}
}

void DevScene::CreateMapButtons ( )
{
	int32 screenW = GWinSizeX;
	int32 screenH = GWinSizeY;

	const int32 btnW = 64;
	const int32 btnH = 40;
	const int32 gap = 7;
	const int32 totalW = btnW * 3 + gap * 2;

	const int32 marginX = 10;
	const int32 marginY = 10;

	int32 startX = screenW - marginX - totalW;
	int32 y = marginY + btnH / 2;

	// 마을1 버튼
	{
		Button* b = new Button ( );
		b->SetSize ( { btnW, btnH } );
		b->SetPos ( { ( float ) ( startX + btnW / 2 ), ( float ) y } );
		b->SetSprite ( GET_SINGLE ( ResourceManager )->GetSprite ( L"Btn_Town1" ) , BS_Default );
		b->SetCurrentSprite ( b->GetSprite ( BS_Default ) );
		b->AddOnClickDelegate ( this , &DevScene::OnClickTown1 );
		_uis.push_back ( b );

		startX += btnW + gap;
	}

	// 마을2 버튼
	{
		Button* b = new Button ( );
		b->SetSize ( { btnW, btnH } );
		b->SetPos ( { ( float ) ( startX + btnW / 2 ), ( float ) y } );
		b->SetSprite ( GET_SINGLE ( ResourceManager )->GetSprite ( L"Btn_Town2" ) , BS_Default );
		b->SetCurrentSprite ( b->GetSprite ( BS_Default ) );
		b->AddOnClickDelegate ( this , &DevScene::OnClickTown2 );
		_uis.push_back ( b );

		startX += btnW + gap;
	}

	// 던전 버튼
	{
		Button* b = new Button ( );
		b->SetSize ( { btnW, btnH } );
		b->SetPos ( { ( float ) ( startX + btnW / 2 ), ( float ) y } );
		b->SetSprite ( GET_SINGLE ( ResourceManager )->GetSprite ( L"Btn_Dungeon" ) , BS_Default );
		b->SetCurrentSprite ( b->GetSprite ( BS_Default ) );
		b->AddOnClickDelegate ( this , &DevScene::OnClickDungeon );
		_uis.push_back ( b );
	}
}

void DevScene::OnClickTown1 ( )
{
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_ChangeMap ( Protocol::MAP_ID_TOWN , 1 );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void DevScene::OnClickTown2 ( )
{
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_ChangeMap ( Protocol::MAP_ID_TOWN , 2 );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void DevScene::OnClickDungeon ( )
{
	SendBufferRef sendBuffer = ClientPacketHandler::Make_C_ChangeMap ( Protocol::MAP_ID_DUNGEON , 0 );
	GET_SINGLE ( NetworkManager )->SendPacket ( sendBuffer );
}

void DevScene::HandlePartyInput ( )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	// 초대 수락/거절 (Y/N)
	if ( myPlayer->_pendingInviteFrom != 0 )
	{
		if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::Y ) )
		{
			SendBufferRef sb = ClientPacketHandler::Make_C_PartyAnswer ( myPlayer->_pendingInviteFrom , true );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
			myPlayer->_pendingInviteFrom = 0;
			myPlayer->_pendingInviterName.clear ( );
			GET_SINGLE ( SoundManager )->Play ( L"UISound" );
		}
		else if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::N ) )
		{
			SendBufferRef sb = ClientPacketHandler::Make_C_PartyAnswer ( myPlayer->_pendingInviteFrom , false );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
			myPlayer->_pendingInviteFrom = 0;
			myPlayer->_pendingInviterName.clear ( );
			GET_SINGLE ( SoundManager )->Play ( L"UISound" );
		}
		return;  // 초대 팝업 중에는 다른 입력 무시
	}

	// P키 파티 탈퇴
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::P ) )
	{
		if ( !myPlayer->_partyMembers.empty ( ) )
		{
			SendBufferRef sb = ClientPacketHandler::Make_C_PartyLeave ( );
			GET_SINGLE ( NetworkManager )->SendPacket ( sb );
		}
	}

	// 좌클릭으로 다른 플레이어 클릭 → 파티 초대
	if ( GET_SINGLE ( InputManager )->GetButtonDown ( KeyType::LeftMouse ) )
	{
		// 인벤토리 드래그 중이면 무시
		if ( _inventoryPanel && _inventoryPanel->IsVisible ( ) && _inventoryPanel->IsDragging ( ) )
			return;

		POINT mouse = GET_SINGLE ( InputManager )->GetMousePos ( );
		Vec2 cameraPos = GET_SINGLE ( SceneManager )->GetCameraPos ( );

		// 마우스 스크린 좌표 → 월드 좌표
		float worldX = mouse.x + cameraPos.x - GWinSizeX / 2.f;
		float worldY = mouse.y + cameraPos.y - GWinSizeY / 2.f;

		uint64 myId = GET_SINGLE ( SceneManager )->GetMyPlayerId ( );

		// LAYER_OBJECT의 Player 순회
		for ( Actor* actor : _actors[ LAYER_OBJECT ] )
		{
			Player* player = dynamic_cast< Player* >( actor );
			if ( player == nullptr )
				continue;
			if ( player->info.objectid ( ) == myId )
				continue;  // 자기 자신 제외
			if ( player->info.objecttype ( ) != Protocol::OBJECT_TYPE_PLAYER )
				continue;

			Vec2 pos = player->GetPos ( );
			float dx = worldX - pos.x;
			float dy = worldY - pos.y;

			// 클릭 범위: 플레이어 중심에서 50px 이내
			if ( dx * dx + dy * dy < 50.f * 50.f )
			{
				SendBufferRef sb = ClientPacketHandler::Make_C_PartyInvite ( player->info.objectid ( ) );
				GET_SINGLE ( NetworkManager )->SendPacket ( sb );
				break;
			}
		}
	}
}

void DevScene::RenderPartyHUD ( HDC hdc )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	if ( myPlayer->_partyMembers.empty ( ) )
		return;

	const int32 startX = 36;
	const int32 startY = 100;
	const int32 rowH = 24;
	const int32 barW = 80;
	const int32 barH = 8;

	Sprite* statusSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"PartyStatus" );
	if ( statusSprite )
	{
		const int32 w = statusSprite->GetSize ( ).x;
		const int32 h = statusSprite->GetSize ( ).y;
		::TransparentBlt ( hdc ,
			startX - 4 , startY - 4 ,
			w , h ,
			statusSprite->GetDC ( ) ,
			statusSprite->GetPos ( ).x , statusSprite->GetPos ( ).y ,
			w , h ,
			statusSprite->GetTransparent ( ) );
	}

	SetBkMode ( hdc , TRANSPARENT );
	SetTextColor ( hdc , RGB ( 0 , 0 , 0 ) );

	// 타이틀
	TextOut ( hdc , startX , startY , L"Party" , 5 );

	for ( int32 i = 0; i < ( int32 ) myPlayer->_partyMembers.size ( ); i++ )
	{
		const auto& member = myPlayer->_partyMembers[ i ];
		int32 y = startY + 18 + i * rowH;

		// 리더 표시 + 이름
		wstring display;
		if ( member.isLeader )
			display = L"* " + member.name;
		else
			display = L"  " + member.name;

		TextOut ( hdc , startX , y , display.c_str ( ) , ( int32 ) display.length ( ) );

		// HP 바
		int32 barX = startX + 100;
		int32 barY = y + 2;

		// 배경 (어두운 빨강)
		HBRUSH darkBrush = CreateSolidBrush ( RGB ( 80 , 0 , 0 ) );
		RECT barBg = { barX , barY , barX + barW , barY + barH };
		FillRect ( hdc , &barBg , darkBrush );
		DeleteObject ( darkBrush );

		// HP 바 (초록)
		if ( member.maxHp > 0 && member.hp > 0 )
		{
			int32 fillW = barW * member.hp / member.maxHp;
			if ( fillW > 0 )
			{
				HBRUSH hpBrush = CreateSolidBrush ( RGB ( 0 , 200 , 0 ) );
				RECT hpRect = { barX , barY , barX + fillW , barY + barH };
				FillRect ( hdc , &hpRect , hpBrush );
				DeleteObject ( hpBrush );
			}
		}
	}
}

void DevScene::RenderPartyInvite ( HDC hdc )
{
	MyPlayer* myPlayer = GET_SINGLE ( SceneManager )->GetMyPlayer ( );
	if ( myPlayer == nullptr )
		return;

	if ( myPlayer->_pendingInviteFrom == 0 )
		return;

	Sprite* inviteSprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"PartyInvite" );
	int32 popW = 300;
	int32 popH = 80;
	if ( inviteSprite )
	{
		popW = inviteSprite->GetSize ( ).x;
		popH = inviteSprite->GetSize ( ).y;
	}
	int32 popX = ( GWinSizeX - popW ) / 2;
	int32 popY = ( GWinSizeY - popH ) / 2 - 50;

	if ( inviteSprite )
	{
		::TransparentBlt ( hdc ,
			popX , popY ,
			popW , popH ,
			inviteSprite->GetDC ( ) ,
			inviteSprite->GetPos ( ).x , inviteSprite->GetPos ( ).y ,
			popW , popH ,
			inviteSprite->GetTransparent ( ) );
	}

	SetBkMode ( hdc , TRANSPARENT );
	SetTextColor ( hdc , RGB ( 50 , 50 , 50 ) );

	// 메시지
	wstring msg = myPlayer->_pendingInviterName + L" invited you to party";
	RECT textRect = { popX + 10 , popY + 15 , popX + popW - 10 , popY + 40 };
	DrawText ( hdc , msg.c_str ( ) , ( int32 ) msg.length ( ) , &textRect , DT_CENTER );

	// Y/N 안내
	wstring hint = L"[Y] Accept    [N] Decline";
	RECT hintRect = { popX + 10 , popY + 45 , popX + popW - 10 , popY + 70 };
	SetTextColor ( hdc , RGB ( 0 , 0 , 0 ) );
	DrawText ( hdc , hint.c_str ( ) , ( int32 ) hint.length ( ) , &hintRect , DT_CENTER );
}

// Load functions

void DevScene::LoadSceneTextures ( )
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

void DevScene::CreateSceneSprites ( )
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

void DevScene::LoadSceneSounds ( )
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

void DevScene::LoadMap ( )
{
	Sprite* sprite = GET_SINGLE ( ResourceManager )->GetSprite ( L"Stage01" );

	SpriteActor* background = new SpriteActor ( );
	background->SetSprite ( sprite );
	background->SetLayer ( LAYER_BACKGROUND );

	const Vec2Int size = sprite->GetSize ( );
	background->SetPos ( Vec2 ( size.x / 2 , size.y / 2 ) );

	AddActor ( background );
	_background = background;
}

void DevScene::LoadPlayer ( )
{
	// IDLE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleUp" );
		fb->SetInfo ( { texture, L"FB_MoveUp", {200, 200}, 0, 9, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleDown" );
		fb->SetInfo ( { texture, L"FB_MoveDown", {200, 200}, 0, 9, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleLeft" );
		fb->SetInfo ( { texture, L"FB_MoveLeft", {200, 200}, 0, 9, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_IdleRight" );
		fb->SetInfo ( { texture, L"FB_MoveRight", {200, 200}, 0, 9, 0, 0.5f } );
	}
	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveUp" );
		fb->SetInfo ( { texture, L"FB_MoveUp", {200, 200}, 0, 9, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveDown" );
		fb->SetInfo ( { texture, L"FB_MoveDown", {200, 200}, 0, 9, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveLeft" );
		fb->SetInfo ( { texture, L"FB_MoveLeft", {200, 200}, 0, 9, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_MoveRight" );
		fb->SetInfo ( { texture, L"FB_MoveRight", {200, 200}, 0, 9, 1, 0.5f } );
	}
	// SKILL
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackUp" );
		fb->SetInfo ( { texture, L"FB_MoveUp", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackDown" );
		fb->SetInfo ( { texture, L"FB_MoveDown", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackLeft" );
		fb->SetInfo ( { texture, L"FB_MoveLeft", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_AttackRight" );
		fb->SetInfo ( { texture, L"FB_MoveRight", {200, 200}, 0, 7, 3, 0.5f, false } );
	}
	// BOW
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowUp" );
		fb->SetInfo ( { texture, L"FB_BowUp", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowDown" );
		fb->SetInfo ( { texture, L"FB_BowDown", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowLeft" );
		fb->SetInfo ( { texture, L"FB_BowLeft", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_BowRight" );
		fb->SetInfo ( { texture, L"FB_BowRight", {200, 200}, 0, 7, 5, 0.5f, false } );
	}
	// STAFF
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerUp" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffUp" );
		fb->SetInfo ( { texture, L"FB_StaffUp", {200, 200}, 0, 10, 6, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerDown" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffDown" );
		fb->SetInfo ( { texture, L"FB_StaffDown", {200, 200}, 0, 10, 6, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerLeft" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffLeft" );
		fb->SetInfo ( { texture, L"FB_StaffLeft", {200, 200}, 0, 10, 6, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"PlayerRight" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_StaffRight" );
		fb->SetInfo ( { texture, L"FB_StaffRight", {200, 200}, 0, 10, 6, 0.5f, false } );
	}

}

void DevScene::LoadMonster ( )
{
	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeUp" );
		fb->SetInfo ( { texture, L"FB_SnakeUp", {100, 100}, 0, 3, 3, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeDown" );
		fb->SetInfo ( { texture, L"FB_SnakeDown", {100, 100}, 0, 3, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeLeft" );
		fb->SetInfo ( { texture, L"FB_SnakeLeft", {100, 100}, 0, 3, 2, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Snake" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_SnakeRight" );
		fb->SetInfo ( { texture, L"FB_SnakeRight", {100, 100}, 0, 3, 1, 0.5f } );
	}

	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocUp" );
		fb->SetInfo ( { texture, L"FB_OctorocUp", {100, 100}, 0, 3, 3, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocDown" );
		fb->SetInfo ( { texture, L"FB_OctorocDown", {100, 100}, 0, 3, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocLeft" );
		fb->SetInfo ( { texture, L"FB_OctorocLeft", {100, 100}, 0, 3, 2, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Octoroc" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_OctorocRight" );
		fb->SetInfo ( { texture, L"FB_OctorocRight", {100, 100}, 0, 3, 1, 0.5f } );
	}
}

void DevScene::LoadProjectiles ( )
{
	// MOVE
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowUp" );
		fb->SetInfo ( { texture, L"FB_ArrowUp", {100, 100}, 0, 0, 3, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowDown" );
		fb->SetInfo ( { texture, L"FB_ArrowDown", {100, 100}, 0, 0, 0, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowLeft" );
		fb->SetInfo ( { texture, L"FB_ArrowLeft", {100, 100}, 0, 0, 1, 0.5f } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Arrow" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_ArrowRight" );
		fb->SetInfo ( { texture, L"FB_ArrowRight", {100, 100}, 0, 0, 2, 0.5f } );
	}
}

void DevScene::LoadEffect ( )
{
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Hit" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_Hit" );
		fb->SetInfo ( { texture, L"FB_Hit", {50, 47}, 0, 5, 0, 0.5f, false } );
	}
	{
		Texture* texture = GET_SINGLE ( ResourceManager )->GetTexture ( L"Explode" );
		Flipbook* fb = GET_SINGLE ( ResourceManager )->CreateFlipbook ( L"FB_Explode" );
		fb->SetInfo ( { texture, L"FB_Explode", {144, 144}, 0, 11, 0, 0.5f, false } );
	}
}

void DevScene::LoadTilemap ( )
{
	TilemapActor* actor = new TilemapActor ( );

	actor->SetLayer ( LAYER_BACKGROUND );
	AddActor ( actor );

	_tilemapActor = actor;
	{
		auto* tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( L"Tilemap_01" );
		tm->SetMapSize ( { 63, 43 } );
		tm->SetTileSize ( 48 );

		GET_SINGLE ( ResourceManager )->LoadTilemap ( L"Tilemap_01" , L"Tilemap\\Tilemap_01.txt" );

		_tilemapActor->SetTilemap ( tm );
		_tilemapActor->SetShowDebug ( false );
	}

	{
		auto* tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( L"Tilemap_02" );
		tm->SetMapSize ( { 40, 32 } );
		tm->SetTileSize ( 48 );

		GET_SINGLE ( ResourceManager )->LoadTilemap ( L"Tilemap_02" , L"Tilemap\\Tilemap_02.txt" );
	}
}

void DevScene::LoadTilemap ( const wchar_t* tilemapFile )
{
	if ( _tilemapActor == nullptr )
	{
		TilemapActor* actor = new TilemapActor ( );
		actor->SetLayer ( LAYER_BACKGROUND );
		AddActor ( actor );
		_tilemapActor = actor;
	}

	const wchar_t* tilemapName = nullptr;
	Vec2Int mapSize = {};

	if ( wcscmp ( tilemapFile , L"Tilemap\\Tilemap_01.txt" ) == 0 )
	{
		tilemapName = L"Tilemap_01";
		mapSize = { 63, 43 };
	}
	else if ( wcscmp ( tilemapFile , L"Tilemap\\Tilemap_02.txt" ) == 0 )
	{
		tilemapName = L"Tilemap_02";
		mapSize = { 40, 32 };
	}
	else
	{
		return;
	}

	Tilemap* tm = GET_SINGLE ( ResourceManager )->GetTilemap ( tilemapName );
	if ( tm == nullptr )
	{
		tm = GET_SINGLE ( ResourceManager )->CreateTilemap ( tilemapName );
		if ( tm == nullptr )
			return;

		tm->SetTileSize ( 48 );
		tm->SetMapSize ( mapSize );
		GET_SINGLE ( ResourceManager )->LoadTilemap ( tilemapName , tilemapFile );
	}

	_tilemapActor->SetTilemap ( tm );
	_tilemapActor->SetShowDebug ( false );
}

