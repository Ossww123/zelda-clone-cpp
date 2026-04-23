#pragma once

class RedisSubscriber
{
public:
    static RedisSubscriber& GetInstance();
    void Start();

private:
    RedisSubscriber() = default;
};

#define GRedisSubscriber RedisSubscriber::GetInstance()