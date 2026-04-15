#pragma once

class RedisSubscriber
{
public:
    static RedisSubscriber& GetInstance();
    void Start();   // 구독 스레드 시작

private:
    RedisSubscriber() = default;
};

#define GRedisSubscriber RedisSubscriber::GetInstance()