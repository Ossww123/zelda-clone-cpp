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
        LOG_INFO("Redis", "ping: %s", pong.c_str());

        _connected = true;
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Redis", "Connect failed: %s — running without Redis", e.what());
        _connected = false;
    }
}

sw::redis::Redis& RedisClient::Get()
{
    return *_redis;
}