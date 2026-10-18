#pragma once

#include <exception>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace mghook::log
{
	/// How serious a log line is.
	enum class Level
	{
		Info,
		Warning,
		Error,
	};

	/// Where the log goes; nothing is written unless one of them is set.
	struct Options
	{
		bool console = false;
		std::filesystem::path file;
	};

	/// Opens the console and the file the options ask for, emptying the file.
	/// @param options Where to write.
	void open(const Options& options);

	/// Tells whether anything is open, so that callers skip building lines nobody reads.
	[[nodiscard]] bool isOpen();

	/// Spells a path in UTF-8, the encoding of the log.
	/// @param path The path.
	[[nodiscard]] std::string pathText(const std::filesystem::path& path);

	/// Writes a line to everything open.
	/// @param level How serious the line is.
	/// @param message The line, without a line break.
	void writeLine(Level level, std::string_view message);

	/// Formats a line and writes it to everything open.
	/// @param level How serious the line is.
	/// @param format The line's format.
	/// @param args What the format fills in.
	template <typename... Args>
	void write(const Level level, std::format_string<Args...> format, Args&&... args) noexcept
	{
		if (!isOpen())
			return;

		// The hook also logs from inside the game's calls, which must never see an exception, and no line is worth one.
		try
		{
			writeLine(level, std::format(format, std::forward<Args>(args)...));
		}
		catch (const std::exception&)
		{
		}
	}

	/// Formats and writes an informational line.
	/// @param format The line's format.
	/// @param args What the format fills in.
	template <typename... Args>
	void info(std::format_string<Args...> format, Args&&... args) noexcept
	{
		write(Level::Info, format, std::forward<Args>(args)...);
	}

	/// Formats and writes a line about something that did not go as expected.
	/// @param format The line's format.
	/// @param args What the format fills in.
	template <typename... Args>
	void warning(std::format_string<Args...> format, Args&&... args) noexcept
	{
		write(Level::Warning, format, std::forward<Args>(args)...);
	}

	/// Formats and writes a line about a part of the hook that could not do its work.
	/// @param format The line's format.
	/// @param args What the format fills in.
	template <typename... Args>
	void error(std::format_string<Args...> format, Args&&... args) noexcept
	{
		write(Level::Error, format, std::forward<Args>(args)...);
	}
}
