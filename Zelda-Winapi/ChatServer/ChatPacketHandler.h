#pragma once
class ChatPacketHandler
{
public:
    static void HandlePacket(ChatSessionRef session, BYTE* buffer, int32 len);

    // �ޱ�
    static void Handle_SS_RelayChat(ChatSessionRef session, BYTE* buffer, int32 len);

    // ������
    static SendBufferRef Make_SS_BroadcastChat(const Protocol::SS_BroadcastChat& pkt);

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