#pragma once

class NetworkManager
{
	DECLARE_SINGLE(NetworkManager )

public:
	void Init ( uint16 port = 7777 );
	void Update ( );

	ServerSessionRef CreateSession ( );
	void SendPacket ( SendBufferRef sendBuffer );

private:
	ClientServiceRef _service;
	ServerSessionRef _session;
};

