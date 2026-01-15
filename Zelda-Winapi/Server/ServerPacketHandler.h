#pragma once

enum
{
	C_Move = 101,
	C_Attack = 102,
	C_ChangeMap = 103,

	S_TEST = 201,
	S_EnterGame = 202,
	S_MyPlayer = 203,
	S_AddObject = 204,
	S_RemoveObject = 205,
	S_Move = 206,
	S_Attack = 207,
	S_Damaged = 208,
	S_ChangeMap = 209,
	// [AUTO-GEN ENUM BEGIN]


	// [AUTO-GEN ENUM END]
};

struct BuffData
{
	uint64 buffId;
	float remainTime;
};

class ServerPacketHandler
{
public:
	static void HandlePacket(GameSessionRef session, BYTE* buffer, int32 len);

	// 받기
	static void Handle_C_Move(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_Attack(GameSessionRef session, BYTE* buffer, int32 len);
	static void Handle_C_ChangeMap(GameSessionRef session, BYTE* buffer, int32 len);

	// 보내기
	static SendBufferRef Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs);
	static SendBufferRef Make_S_EnterGame();
	static SendBufferRef Make_S_MyPlayer(const Protocol::ObjectInfo info);
	static SendBufferRef Make_S_AddObject(const Protocol::S_AddObject& pkt);
	static SendBufferRef Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt);
	static SendBufferRef Make_S_Move(const Protocol::ObjectInfo& info);
	static SendBufferRef Make_S_Attack(const Protocol::S_Attack& pkt);
	static SendBufferRef Make_S_Damaged(const Protocol::S_Damaged& pkt);
	static SendBufferRef Make_S_ChangeMap(const Protocol::S_ChangeMap& pkt);
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

