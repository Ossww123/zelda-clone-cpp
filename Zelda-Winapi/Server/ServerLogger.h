#pragma once
#include <fstream>
#include <cstdarg>

class ServerLogger
{
public:
	static ServerLogger& GetInstance();

	void Write(const string& level, const string& tag, const string& msg);

private:
	ServerLogger() = default;

	void EnsureFileOpen();
	string GetTimestamp();
	string GetDateString();

	ofstream _serverLog;
	string   _lastDate;
	USE_LOCK;
};

#define GServerLogger ServerLogger::GetInstance()

inline string LogFormat(const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return buf;
}

#define LOG_INFO(tag, fmt, ...)  GServerLogger.Write("INFO ", tag, LogFormat(fmt, ##__VA_ARGS__))
#define LOG_WARN(tag, fmt, ...)  GServerLogger.Write("WARN ", tag, LogFormat(fmt, ##__VA_ARGS__))
#define LOG_ERROR(tag, fmt, ...) GServerLogger.Write("ERROR", tag, LogFormat(fmt, ##__VA_ARGS__))
