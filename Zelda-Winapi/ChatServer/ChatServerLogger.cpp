#include "pch.h"
#include "ChatServerLogger.h"

ChatServerLogger& ChatServerLogger::GetInstance()
{
	static ChatServerLogger instance;
	return instance;
}

void ChatServerLogger::Write(const string& level, const string& tag, const string& msg)
{
	WRITE_LOCK;

	EnsureFileOpen();

	string paddedTag = tag;
	if (paddedTag.size() < 7) paddedTag.resize(7, ' ');
	else if (paddedTag.size() > 7) paddedTag = paddedTag.substr(0, 7);

	string line = GetTimestamp() + "[" + level + "][" + paddedTag + "] " + msg;

	if (_serverLog.is_open())
		_serverLog << line << "\n" << flush;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (level == "ERROR")
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
	else if (level == "WARN ")
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	cout << line << "\n";

	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void ChatServerLogger::WriteChat(const string& sender, const string& type,
	const string& target, const string& msg)
{
	WRITE_LOCK;

	EnsureFileOpen();

	string line = GetTimestamp() + "[CHAT] " + type + " | sender=" + sender;
	if (!target.empty())
		line += " | to=" + target;
	line += " | msg=" + msg;

	if (_chatLog.is_open())
		_chatLog << line << "\n" << flush;

	cout << line << "\n";
}

void ChatServerLogger::EnsureFileOpen()
{
	string today = GetDateString();
	if (today == _lastDate && _serverLog.is_open())
		return;

	if (_serverLog.is_open()) _serverLog.close();
	if (_chatLog.is_open())   _chatLog.close();

	_lastDate = today;
	CreateDirectoryA("logs", nullptr);
	_serverLog.open("logs/server_" + today + ".log", ios::app);
	_chatLog.open("logs/chat_"     + today + ".log", ios::app);
}

string ChatServerLogger::GetTimestamp()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buf[32];
	snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d]",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	return buf;
}

string ChatServerLogger::GetDateString()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buf[16];
	snprintf(buf, sizeof(buf), "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
	return buf;
}
