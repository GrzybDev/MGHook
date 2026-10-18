#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mghook
{
	/// A key=value line of a translation or character map file.
	struct TextEntry
	{
		std::string key;
		std::string value;
		int line;
	};

	/// The entries of a text file and a note on each line it skipped.
	struct TextFile
	{
		std::vector<TextEntry> entries;
		std::vector<std::string> problems;
	};

	/// Parses the UTF-8 key=value lines of the hook's text files, noting each line it skips.
	/// @param content The file's bytes, with or without a byte order mark.
	[[nodiscard]] TextFile parseTextFile(std::string_view content);

	/// Reads and parses a text file.
	/// @param path The file.
	/// @return The parsed file, or nothing when it cannot be opened.
	[[nodiscard]] std::optional<TextFile> readTextFile(const std::filesystem::path& path);
}
