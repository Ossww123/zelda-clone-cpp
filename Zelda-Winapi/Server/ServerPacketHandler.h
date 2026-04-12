#pragma once

enum
{
	C_Move = 101,
	C_Attack = 102,
	C_ChangeMap = 103,
	C_Turn = 104,
	C_EquipItem = 105,
	C_UnequipItem = 106,
	C_UseItem = 107,
	C_PartyAnswer = 109,
	C_PartyInvite = 108,
	C_PartyLeave = 110,
	C_Login = 111,
	C_Chat = 112,

	S_TEST = 201,
	S_EnterGame = 202,
	S_MyPlayer = 203,
	S_AddObject = 204,
	S_RemoveObject = 205,
	S_Move = 206,
	S_Attack = 207,
	S_Damaged = 208,
	S_ChangeMap = 209,
	S_GainExp = 210,
	S_LevelUp = 211,
	S_Turn = 212,
	S_AddItem = 214,
	S_EquipItem = 215,
	S_InventoryData = 213,
	S_UnequipItem = 216,
	S_UseItem = 217,
	S_PartyInvite = 218,
	S_PartyLeave = 220,
	S_PartyUpdate = 219,
	S_Chat = 221,

	// [AUTO-GEN ENUM BEGIN]


	// [AUTO-GEN ENUM END]
};

struct BuffData
{
	uint64 buffId;
	float remainTime;
};

struct PacketPerfSnapshot
{
	uint64 recvMove = 0;
	uint64 recvAttack = 0;
	uint64 recvTurn = 0;
};

class ServerPacketHandler
{
public:
	static void HandlePacket(GameSessionRef session, BYTE* buffer, int32 len);
	static PacketPerfSnapshot ConsumePerfSnapshot();

	// 받기
	static void Handle_C_Move(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_Attack(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_ChangeMap(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_Turn(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_EquipItem(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_UnequipItem(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_UseItem(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_PartyInvite(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_PartyAnswer(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_PartyLeave(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_Login(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_Chat(GameSessionRef session, BYTE* buffer, int32 len);

	// 보내기
	static SendBufferRef Make_S_EnterGame();
	static SendBufferRef Make_S_MyPlayer(const Protocol::S_MyPlayer& pkt);
	static SendBufferRef Make_S_AddObject(const Protocol::S_AddObject& pkt);
	static SendBufferRef Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt);
	static SendBufferRef Make_S_Move(const Protocol::S_Move& pkt);
	static SendBufferRef Make_S_Attack(const Protocol::S_Attack& pkt);
	static SendBufferRef Make_S_Damaged(const Protocol::S_Damaged& pkt);
	static SendBufferRef Make_S_ChangeMap(const Protocol::S_ChangeMap& pkt);
	static SendBufferRef Make_S_GainExp(const Protocol::S_GainExp& pkt);
	static SendBufferRef Make_S_LevelUp(const Protocol::S_LevelUp& pkt);
	static SendBufferRef Make_S_Turn(const Protocol::S_Turn& pkt);
	static SendBufferRef Make_S_InventoryData(const Protocol::S_InventoryData& pkt);
	static SendBufferRef Make_S_AddItem(const Protocol::S_AddItem& pkt);
	static SendBufferRef Make_S_EquipItem(const Protocol::S_EquipItem& pkt);
	static SendBufferRef Make_S_UnequipItem(const Protocol::S_UnequipItem& pkt);
	static SendBufferRef Make_S_UseItem(const Protocol::S_UseItem& pkt);
	static SendBufferRef Make_S_PartyInvite(const Protocol::S_PartyInvite& pkt);
	static SendBufferRef Make_S_PartyUpdate(const Protocol::S_PartyUpdate& pkt);
	static SendBufferRef Make_S_PartyLeave();
	static SendBufferRef Make_S_Chat(const Protocol::S_Chat& pkt);
	// [AUTO-GEN DECLS BEGIN]


	
	

	// [AUTO-GEN DECLS END]

	template<typename T>
	static SendBufferRef MakeSendBuffer(const T& pkt, uint16 pktId)
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

