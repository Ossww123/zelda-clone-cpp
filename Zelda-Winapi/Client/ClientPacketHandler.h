#pragma once

enum
{
	C_Move = 101 ,
	C_Attack = 102 ,
	C_ChangeMap = 103 ,
	C_Turn = 104 ,
	C_EquipItem = 105 ,
	C_UnequipItem = 106 ,
	C_UseItem = 107 ,
	C_PartyAnswer = 109 ,
	C_PartyInvite = 108 ,
	C_PartyLeave = 110 ,
	C_Login = 111 ,

	S_TEST = 201 ,
	S_EnterGame = 202 ,
	S_MyPlayer = 203 ,
	S_AddObject = 204 ,
	S_RemoveObject = 205 ,
	S_Move = 206 ,
	S_Attack = 207 ,
	S_Damaged = 208 ,
	S_ChangeMap = 209 ,
	S_GainExp = 210 ,
	S_LevelUp = 211 ,
	S_Turn = 212 ,
	S_AddItem = 214 ,
	S_EquipItem = 215 ,
	S_InventoryData = 213 ,
	S_UnequipItem = 216 ,
	S_UseItem = 217 ,
	S_PartyInvite = 218 ,
	S_PartyLeave = 220 ,
	S_PartyUpdate = 219 ,
	// [AUTO-GEN ENUM BEGIN]

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
	static void Handle_S_Turn ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_InventoryData ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_AddItem ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_EquipItem ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_UnequipItem ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_UseItem ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_PartyInvite ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_PartyUpdate ( ServerSessionRef session , BYTE* buffer , int32 len );
	static void Handle_S_PartyLeave ( ServerSessionRef session , BYTE* buffer , int32 len );

	// 보내기
	static SendBufferRef Make_C_Move ( Protocol::DIR_TYPE dir );
	static SendBufferRef Make_C_Attack ( Protocol::DIR_TYPE dir , Protocol::WEAPON_TYPE weapon );
	static SendBufferRef Make_C_ChangeMap ( const Protocol::MAP_ID& mapId , int32 channel );
	static SendBufferRef Make_C_Turn ( const Protocol::DIR_TYPE& dir );
	static SendBufferRef Make_C_EquipItem ( int32 slot );
	static SendBufferRef Make_C_UnequipItem ( int32 equipType );
	static SendBufferRef Make_C_UseItem ( int32 slot );
	static SendBufferRef Make_C_PartyInvite ( uint64 targetId );
	static SendBufferRef Make_C_PartyAnswer ( uint64 inviterId , bool accept );
	static SendBufferRef Make_C_PartyLeave ( );
	static SendBufferRef Make_C_Login ( const string& username );

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

