#pragma once
#include <fstream>
#include <cstdarg>

class ChatServerLogger
{
public:
	static ChatServerLogger& GetInstance();

	void Write(const string& level, const string& tag, const string& msg);
	void WriteChat(const string& sender, const string& type,
		const string& target, const string& msg);

private:
	ChatServerLogger() = default;

	void EnsureFileOpen();
	string GetTimestamp();
	string GetDateString();

	ofstream _serverLog;
	ofstream _chatLog;
	string   _lastDate;
	USE_LOCK;
};

#define GChatServerLogger ChatServerLogger::GetInstance()

inline string LogFormat(const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return buf;
}

#define LOG_INFO(tag, fmt, ...)             GChatServerLogger.Write("INFO ", tag, LogFormat(fmt, ##__VA_ARGS__))
#define LOG_WARN(tag, fmt, ...)             GChatServerLogger.Write("WARN ", tag, LogFormat(fmt, ##__VA_ARGS__))
#define LOG_ERROR(tag, fmt, ...)            GChatServerLogger.Write("ERROR", tag, LogFormat(fmt, ##__VA_ARGS__))
#define LOG_CHAT(sender, type, target, msg) GChatServerLogger.WriteChat(sender, type, target, msg)
