#include "pch.h"
#include "RedisClient.h"

RedisClient& RedisClient::GetInstance()
{
    static RedisClient instance;
    return instance;
}

void RedisClient::Connect(const std::string& host, int port)
{
    sw::redis::ConnectionOptions opts;
    opts.host = host;
    opts.port = port;

    _redis = std::make_unique<sw::redis::Redis>(opts);

    auto pong = _redis->ping();
    std::cout << "Redis ping: " << pong << std::endl;

    _connected = true;
}

sw::redis::Redis& RedisClient::Get()
{
    return *_redis;
}