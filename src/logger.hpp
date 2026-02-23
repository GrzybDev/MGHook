#pragma once

class Logger
{
public:
	static void Init(HMODULE hModule);
	static void Log(const char* format, ...);

private:
	Logger() = default;
	static Logger& GetInstance();

	std::wstring logFilePath;

	bool isConsoleEnabled = false;
	bool isFileLoggingEnabled = false;

	static void InitConsole();
};
