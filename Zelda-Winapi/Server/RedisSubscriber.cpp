#include "pch.h"
#include "RedisSubscriber.h"
#include "RedisClient.h"
#include "GameSessionManager.h"
#include "GameSession.h"
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
            if (!GRedisClient.IsConnected())
            {
                cout << "[RedisSubscriber] Redis unavailable, subscriber not started" << endl;
                return;
            }

            // 구독 전용 독립 Redis 연결 (GRedisClient 풀과 분리)
            sw::redis::ConnectionOptions opts;
            opts.host = "127.0.0.1";
            opts.port = 6379;
            sw::redis::Redis subRedis(opts);
            auto sub = subRedis.subscriber();
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

                    if (broadcast.type() == Protocol::CHAT_TYPE_WHISPER)
                    {
                        GameSessionRef target = GSessionManager.FindByPlayerName(broadcast.target());
                        if (target)
                            target->Send(sendBuffer);
                    }
                    else
                    {
                        GSessionManager.Broadcast(sendBuffer);
                    }
                });

            sub.subscribe("chat:global");
            sub.subscribe("chat:whisper:" + GServerAddr);

            while (true)
            {
                sub.consume();
            }
        }).detach();
}