#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

static std::atomic<uint64> GRecvEnterGame = 0;
static std::atomic<uint64> GRecvMove = 0;
static std::atomic<uint64> GRecvAttack = 0;
static std::atomic<uint64> GRecvDamaged = 0;
static std::atomic<uint64> GRecvTurn = 0;

void ClientPacketHandler::HandlePacket(BYTE* buffer, int32 len)
{
    BufferReader br(buffer, len);

    PacketHeader header;
    br >> header;

    switch (header.id)
    {
    case S_TEST:
        Handle_S_TEST(buffer, len);
        break;
    case S_EnterGame:
        Handle_S_EnterGame(buffer, len);
        break;
    case S_Move:
        Handle_S_Move(buffer, len);
        break;
    case S_Attack:
        Handle_S_Attack(buffer, len);
        break;
    case S_Damaged:
        Handle_S_Damaged(buffer, len);
        break;
    case S_Turn:
        Handle_S_Turn(buffer, len);
        break;
    default:
        break;
    }
}

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    uint16 size = header->size;

    Protocol::S_TEST pkt;
    pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
}

void ClientPacketHandler::Handle_S_EnterGame(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    uint16 size = header->size;

    Protocol::S_EnterGame pkt;
    if (pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader)))
        GRecvEnterGame.fetch_add(1);
}

void ClientPacketHandler::Handle_S_Move(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    uint16 size = header->size;

    Protocol::S_Move pkt;
    if (pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader)))
        GRecvMove.fetch_add(1);
}

void ClientPacketHandler::Handle_S_Attack(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    uint16 size = header->size;

    Protocol::S_Attack pkt;
    if (pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader)))
        GRecvAttack.fetch_add(1);
}

void ClientPacketHandler::Handle_S_Damaged(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    uint16 size = header->size;

    Protocol::S_Damaged pkt;
    if (pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader)))
        GRecvDamaged.fetch_add(1);
}

void ClientPacketHandler::Handle_S_Turn(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    uint16 size = header->size;

    Protocol::S_Turn pkt;
    if (pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader)))
        GRecvTurn.fetch_add(1);
}

SendBufferRef ClientPacketHandler::Make_C_Login(const std::string& username)
{
    Protocol::C_Login pkt;
    pkt.set_username(username);
    return MakeSendBuffer(pkt, C_Login);
}

SendBufferRef ClientPacketHandler::Make_C_Move(Protocol::DIR_TYPE dir)
{
    Protocol::C_Move pkt;
    pkt.set_dir(dir);
    return MakeSendBuffer(pkt, C_Move);
}

SendBufferRef ClientPacketHandler::Make_C_Attack(Protocol::DIR_TYPE dir, Protocol::WEAPON_TYPE weaponType)
{
    Protocol::C_Attack pkt;
    pkt.set_dir(dir);
    pkt.set_weapontype(weaponType);
    return MakeSendBuffer(pkt, C_Attack);
}

DummyClientStats ClientPacketHandler::ConsumeStats()
{
    DummyClientStats stats;
    stats.recvEnterGame = GRecvEnterGame.exchange(0);
    stats.recvMove = GRecvMove.exchange(0);
    stats.recvAttack = GRecvAttack.exchange(0);
    stats.recvDamaged = GRecvDamaged.exchange(0);
    stats.recvTurn = GRecvTurn.exchange(0);
    return stats;
}
