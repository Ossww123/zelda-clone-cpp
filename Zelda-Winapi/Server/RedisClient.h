#pragma once
#include <sw/redis++/redis++.h>

class RedisClient
{
public:
    static RedisClient& GetInstance();

    void Connect(const std::string& host = "127.0.0.1", int port = 6379);
    sw::redis::Redis& Get();
    bool IsConnected() const { return _connected; }

private:
    RedisClient() = default;

    std::unique_ptr<sw::redis::Redis> _redis;
    bool _connected = false;
};

#define GRedisClient RedisClient::GetInstance()