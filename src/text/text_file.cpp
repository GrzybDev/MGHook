#include "pch.hpp"
#include "text_file.hpp"

#include "util/files.hpp"

namespace mghook
{
	namespace
	{
		constexpr std::string_view kByteOrderMark = "\xEF\xBB\xBF";

		struct Field
		{
			std::string text;
			std::size_t end;
		};

		int hexDigit(const char digit)
		{
			if (digit >= '0' && digit <= '9')
				return digit - '0';
			if (digit >= 'a' && digit <= 'f')
				return digit - 'a' + 10;
			if (digit >= 'A' && digit <= 'F')
				return digit - 'A' + 10;
			return -1;
		}

		// A key ends at its first unescaped =, a value at the end of the line.
		Field readField(const std::string_view line, std::size_t start, const bool isKey)
		{
			std::string text;
			while (start < line.size() && !(isKey && line[start] == '='))
			{
				if (line[start] != '\\')
				{
					text += line[start++];
					continue;
				}

				const char escape = start + 1 < line.size() ? line[start + 1] : '\0';
				switch (escape)
				{
				case 'n':
					text += '\n';
					break;
				case 'r':
					text += '\r';
					break;
				case 't':
					text += '\t';
					break;
				case '\\':
				case '=':
					text += escape;
					break;
				case 'x':
				{
					const int high = start + 2 < line.size() ? hexDigit(line[start + 2]) : -1;
					const int low = start + 3 < line.size() ? hexDigit(line[start + 3]) : -1;
					if (high < 0 || low < 0)
						throw std::invalid_argument("\\x needs two hexadecimal digits");

					text += static_cast<char>(high << 4 | low);
					start += 2;
					break;
				}
				default:
					throw std::invalid_argument(escape ? std::format("\\{} is no escape", escape) : "it ends with \\");
				}

				start += 2;
			}

			return {.text = std::move(text), .end = start};
		}
	}

	TextFile parseTextFile(std::string_view content)
	{
		if (content.starts_with(kByteOrderMark))
			content.remove_prefix(kByteOrderMark.size());

		TextFile file;
		std::unordered_map<std::string, std::size_t> indexOfKey;
		int number = 0;
		while (!content.empty())
		{
			const std::size_t lineEnd = std::min(content.find('\n'), content.size());
			std::string_view line = content.substr(0, lineEnd);
			content.remove_prefix(std::min(lineEnd + 1, content.size()));
			++number;

			if (line.ends_with('\r'))
				line.remove_suffix(1);
			if (line.empty() || line.starts_with('#') || line.starts_with(';'))
				continue;

			try
			{
				Field key = readField(line, 0, true);
				if (key.end == line.size())
					throw std::invalid_argument("it has no =");
				if (key.text.empty())
					throw std::invalid_argument("the key is empty");

				Field value = readField(line, key.end + 1, false);
				if (value.text.empty())
					throw std::invalid_argument("the value is empty");

				TextEntry entry{.key = std::move(key.text), .value = std::move(value.text), .line = number};
				if (const auto known = indexOfKey.find(entry.key); known != indexOfKey.end())
				{
					TextEntry& earlier = file.entries[known->second];
					file.problems.push_back(std::format("line {}: replaces the same key on line {}", number, earlier.line));
					earlier = std::move(entry);
					continue;
				}

				indexOfKey.emplace(entry.key, file.entries.size());
				file.entries.push_back(std::move(entry));
			}
			catch (const std::invalid_argument& error)
			{
				file.problems.push_back(std::format("line {}: skipped, {}", number, error.what()));
			}
		}

		return file;
	}

	std::optional<TextFile> readTextFile(const std::filesystem::path& path)
	{
		const std::optional<std::string> content = readFile(path);
		return content ? std::optional(parseTextFile(*content)) : std::nullopt;
	}
}
