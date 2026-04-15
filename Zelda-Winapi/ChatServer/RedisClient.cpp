#include "pch.h"
#include "RedisClient.h"

RedisClient& RedisClient::GetInstance()
{
    static RedisClient instance;
    return instance;
}

void RedisClient::Connect(const std::string& host, int port)
{
    try
    {
        sw::redis::ConnectionOptions opts;
        opts.host = host;
        opts.port = port;

        _redis = std::make_unique<sw::redis::Redis>(opts);

        auto pong = _redis->ping();
        std::cout << "[Redis] ping: " << pong << std::endl;

        _connected = true;
    }
    catch (const std::exception& e)
    {
        std::cout << "[Redis] Connect failed: " << e.what() << " — running without Redis" << std::endl;
        _connected = false;
    }
}

sw::redis::Redis& RedisClient::Get()
{
    return *_redis;
}