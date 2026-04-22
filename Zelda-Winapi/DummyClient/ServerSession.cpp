#include "pch.h"
#include "ServerSession.h"
#include "ClientPacketHandler.h"

ServerSession::ServerSession(int32 botId, int32 moveIntervalMs, int32 attackIntervalMs)
    : _botId(botId), _moveIntervalMs(moveIntervalMs), _attackIntervalMs(attackIntervalMs)
{
    _rng.seed(static_cast<uint32>(GetTickCount64()) + static_cast<uint32>(_botId));
}

void ServerSession::OnConnected()
{
    string username = "bot_" + to_string(_botId);
    Protocol::C_Login loginPkt;
    loginPkt.set_username(username);
    Send(ClientPacketHandler::Make_C_Login(loginPkt));

    _nextMoveAt = chrono::steady_clock::now();
    _nextAttackAt = chrono::steady_clock::now();
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
    if (header->id == S_EnterGame)
    {
        Protocol::S_EnterGame pkt;
        if (pkt.ParseFromArray(&header[1], header->size - sizeof(PacketHeader)) && pkt.success())
            _loggedIn.store(true);
    }

    ClientPacketHandler::HandlePacket(buffer, len);
}

void ServerSession::Tick(const chrono::steady_clock::time_point& now)
{
    if (!_loggedIn.load())
        return;

    if (now >= _nextMoveAt)
    {
        Protocol::DIR_TYPE dir = static_cast<Protocol::DIR_TYPE>(_dirDist(_rng));
        Protocol::C_Move movePkt;
        movePkt.set_dir(dir);
        Send(ClientPacketHandler::Make_C_Move(movePkt));
        _nextMoveAt = now + chrono::milliseconds(_moveIntervalMs);
    }

    if (now >= _nextAttackAt)
    {
        Protocol::DIR_TYPE dir = static_cast<Protocol::DIR_TYPE>(_dirDist(_rng));
        Protocol::C_Attack attackPkt;
        attackPkt.set_dir(dir);
        attackPkt.set_weapontype(Protocol::WEAPON_TYPE_SWORD);
        Send(ClientPacketHandler::Make_C_Attack(attackPkt));
        _nextAttackAt = now + chrono::milliseconds(_attackIntervalMs);
    }
}
