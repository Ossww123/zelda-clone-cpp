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
    C_PartyInvite = 108,
    C_PartyAnswer = 109,
    C_PartyLeave = 110,
    C_Login = 111,

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
    S_InventoryData = 213,
    S_AddItem = 214,
    S_EquipItem = 215,
    S_UnequipItem = 216,
    S_UseItem = 217,
    S_PartyInvite = 218,
    S_PartyUpdate = 219,
    S_PartyLeave = 220,
};

struct DummyClientStats
{
    uint64 recvEnterGame = 0;
    uint64 recvMove = 0;
    uint64 recvAttack = 0;
    uint64 recvDamaged = 0;
    uint64 recvTurn = 0;
};

class ClientPacketHandler
{
public:
    static void HandlePacket(BYTE* buffer, int32 len);

    // receive
    static void Handle_S_TEST(BYTE* buffer, int32 len);
    static void Handle_S_EnterGame(BYTE* buffer, int32 len);
    static void Handle_S_Move(BYTE* buffer, int32 len);
    static void Handle_S_Attack(BYTE* buffer, int32 len);
    static void Handle_S_Damaged(BYTE* buffer, int32 len);
    static void Handle_S_Turn(BYTE* buffer, int32 len);

    // send
    static SendBufferRef Make_C_Login(const std::string& username);
    static SendBufferRef Make_C_Move(Protocol::DIR_TYPE dir);
    static SendBufferRef Make_C_Attack(Protocol::DIR_TYPE dir, Protocol::WEAPON_TYPE weaponType);

    // stats
    static DummyClientStats ConsumeStats();

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
