#include "pch.hpp"
#include "log.hpp"

namespace mghook::log
{
	namespace
	{
		std::mutex gMutex;
		FILE* gConsole = nullptr;
		FILE* gFile = nullptr;

		FILE* attachConsole()
		{
			if (!AllocConsole())
				return nullptr;

			// Closing a console ends the process it belongs to, which here is the game.
			DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND);
			SetConsoleOutputCP(CP_UTF8);
			FILE* stream = nullptr;
			return freopen_s(&stream, "CONOUT$", "w", stdout) == 0 ? stream : nullptr;
		}

		std::string_view label(const Level level)
		{
			switch (level)
			{
			case Level::Warning:
				return "Warning";
			case Level::Error:
				return "Error";
			default:
				return "Info";
			}
		}
	}

	void open(const Options& options)
	{
		const std::scoped_lock lock(gMutex);

		if (options.console)
			gConsole = attachConsole();

		if (!options.file.empty() && _wfopen_s(&gFile, options.file.c_str(), L"w") != 0)
			gFile = nullptr;
	}

	bool isOpen()
	{
		return gConsole || gFile;
	}

	std::string pathText(const std::filesystem::path& path)
	{
		const std::u8string text = path.u8string();
		return {text.begin(), text.end()};
	}

	void writeLine(const Level level, const std::string_view message)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		const std::string line = std::format("[{:02}:{:02}:{:02}.{:03}][{}] {}\n", time.wHour, time.wMinute,
		                                     time.wSecond, time.wMilliseconds, label(level), message);

		const std::scoped_lock lock(gMutex);
		for (FILE* stream : {gConsole, gFile})
		{
			if (stream)
			{
				std::fwrite(line.data(), 1, line.size(), stream);
				std::fflush(stream);
			}
		}
	}
}
