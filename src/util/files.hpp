#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mghook
{
	/// Reads a whole file as bytes.
	/// @param path The file to read.
	/// @return The contents, or nothing when the file cannot be opened.
	inline std::optional<std::string> readFile(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
			return std::nullopt;

		return std::string(std::istreambuf_iterator<char>(stream), {});
	}

	/// Writes a whole file, creating its folders.
	/// @param path The file to write.
	/// @param content The bytes to write.
	/// @throws std::runtime_error when the file cannot be written.
	inline void writeFile(const std::filesystem::path& path, const std::string_view content)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream stream(path, std::ios::binary);
		stream.write(content.data(), static_cast<std::streamsize>(content.size()));
		if (!stream)
			throw std::runtime_error("The file could not be written");
	}
}
