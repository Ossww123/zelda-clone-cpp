#include "pch.h"
#include "ServerLogger.h"

ServerLogger& ServerLogger::GetInstance()
{
	static ServerLogger instance;
	return instance;
}

void ServerLogger::Write(const string& level, const string& tag, const string& msg)
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

void ServerLogger::EnsureFileOpen()
{
	string today = GetDateString();
	if (today == _lastDate && _serverLog.is_open())
		return;

	if (_serverLog.is_open())
		_serverLog.close();

	_lastDate = today;
	CreateDirectoryA("logs", nullptr);
	_serverLog.open("logs/server_" + today + ".log", ios::app);
}

string ServerLogger::GetTimestamp()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buf[32];
	snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d]",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	return buf;
}

string ServerLogger::GetDateString()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	char buf[16];
	snprintf(buf, sizeof(buf), "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
	return buf;
}
