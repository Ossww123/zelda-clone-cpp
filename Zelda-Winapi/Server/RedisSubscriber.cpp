#include "pch.h"
#include "RedisSubscriber.h"
#include "RedisClient.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"

RedisSubscriber& RedisSubscriber::GetInstance()
{
    static RedisSubscriber instance;
    return instance;
}

void RedisSubscriber::Start()
{
    thread([this]()
        {
            auto sub = GRedisClient.Get().subscriber();
            sub.on_message([](string channel, string payload)
                {
                    Protocol::SS_BroadcastChat broadcast;
                    if (!broadcast.ParseFromString(payload))
                        return;

                    Protocol::S_Chat chat;
                    chat.set_sender(broadcast.sender());
                    chat.set_type(broadcast.type());
                    chat.set_msg(broadcast.msg());

                    SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Chat(chat);
                    GSessionManager.Broadcast(sendBuffer);
                });

            sub.subscribe("chat:global");
            sub.subscribe("chat:whisper:127.0.0.1:7777");

            while (true)
            {
                sub.consume();
            }
        }).detach();
}