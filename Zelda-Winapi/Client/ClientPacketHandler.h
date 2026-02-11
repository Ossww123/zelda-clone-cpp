#pragma once

enum
{
	C_Move = 101 ,
	C_Attack = 102 ,
	C_ChangeMap = 103 ,

	S_TEST = 201 ,
	S_EnterGame = 202 ,
	S_MyPlayer = 203 ,
	S_AddObject = 204 ,
	S_RemoveObject = 205 ,
	S_Move = 206 ,
	S_Attack = 207 ,
	S_Damaged = 208 ,
	S_ChangeMap = 209 ,
	// [AUTO-GEN ENUM BEGIN]
	S_GainExp = 210 ,
	S_LevelUp = 211 ,



	// [AUTO-GEN ENUM END]
};

class ClientPacketHandler
{
public:
	static void HandlePacket(ServerSessionRef session, BYTE* buffer, int32 len);

	// 받기
	static void Handle_S_TEST( ServerSessionRef session , BYTE* buffer, int32 len);
	static void Handle_S_EnterGame ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_MyPlayer ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_AddObject ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_RemoveObject ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_Move ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_Attack ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_Damaged ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_ChangeMap ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_GainExp ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_LevelUp ( ServerSessionRef session , BYTE* buffer , int32 len );

	// 보내기
	static SendBufferRef Make_C_Move ( Protocol::DIR_TYPE dir , int32 x , int32 y );
	static SendBufferRef Make_C_Attack ( Protocol::DIR_TYPE dir , Protocol::WEAPON_TYPE weapon );
	static SendBufferRef Make_C_ChangeMap ( const Protocol::MAP_ID& mapId , int32 channel );
	// [AUTO-GEN DECLS BEGIN]

	// [AUTO-GEN DECLS END]

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		assert(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};

