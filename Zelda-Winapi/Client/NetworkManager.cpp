#include "pch.h"
#include "NetworkManager.h"
#include "Service.h"
#include "ThreadManager.h"
#include "ServerSession.h"

void NetworkManager::Init ( uint16 port )
{
	SocketUtils::Init ( );

	_service = make_shared<ClientService> (
		NetAddress ( L"127.0.0.1" , port ) ,
		make_shared<IocpCore> ( ) ,
		[=] ( ) { return CreateSession ( ); } , // 클라이언트가 GameServer 외 다른 서버(ChatServer 등)에도 직접 접속하는 구조로 바뀌면 SessionManager로 다중 세션 관리 필요
		1 );

	assert ( _service->Start ( ) );

	/*
	for ( int32 i = 0; i < 5; i++ )
	{
		GThreadManager->Launch ( [ = ] ( )
			{
				while ( true )
				{
					service->GetIocpCore ( )->Dispatch ( );
				}
			} );
	}
	*/
}

void NetworkManager::Update ( )
{
	_service->GetIocpCore ( )->Dispatch ( 0 );
}

ServerSessionRef NetworkManager::CreateSession ( )
{
	return _session = make_shared<ServerSession>();
}

void NetworkManager::SendPacket ( SendBufferRef sendBuffer )
{
	if ( _session )
		_session->Send ( sendBuffer );
}
