#include "pch.hpp"
#include "logger.hpp"


Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

void Logger::Init(HMODULE hModule)
{
	Logger& instance = GetInstance();

	std::ifstream debugFile(".debug");
	instance.isConsoleEnabled = debugFile.good();

	std::ifstream logFile(".debug_log");
	instance.isFileLoggingEnabled = logFile.good();

	if (instance.isConsoleEnabled)
	{
		InitConsole();
	}

	if (instance.isFileLoggingEnabled)
	{
		instance.logFilePath = GetDllDirectory(hModule) + L"mghook.log";
		DeleteFileW(instance.logFilePath.c_str());
	}
}

void Logger::InitConsole()
{
	// Create a console for Debug output
	AllocConsole();

	// Redirect standard error, output to console
	// std::cout, std::clog, std::cerr, std::cin
	FILE* fDummy;

	(void)freopen_s(&fDummy, "CONOUT$", "w", stdout);
	(void)freopen_s(&fDummy, "CONOUT$", "w", stderr);
	(void)freopen_s(&fDummy, "CONIN$", "r", stdin);

	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// Redirect wide standard error, output to console
	// std::wcout, std::wclog, std::wcerr, std::wcin
	const HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	const HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);

	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
}

void Logger::Log(const char* format, ...)
{
	Logger& instance = GetInstance();

	va_list args;
	va_start(args, format);

	if (instance.isConsoleEnabled)
	{
		vprintf(format, args);
	}

	if (instance.isFileLoggingEnabled)
	{
		FILE* file = nullptr;
		if (_wfopen_s(&file, instance.logFilePath.c_str(), L"a") == 0 && file)
		{
			vfprintf(file, format, args);
			fclose(file);
		}
	}

	va_end(args);
}
