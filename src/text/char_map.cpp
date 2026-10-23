#include "pch.hpp"
#include "char_map.hpp"

namespace mghook
{
	namespace
	{
		// A byte that starts no valid UTF-8 character counts as a character of its own.
		std::size_t characterLength(const std::string_view text)
		{
			const auto lead = static_cast<unsigned char>(text[0]);
			const std::size_t length = lead < 0x80 ? 1 : lead >> 5 == 0x6 ? 2 : lead >> 4 == 0xE ? 3 : lead >> 3 == 0x1E ? 4 : 0;
			if (length == 0 || length > text.size())
				return 1;

			const bool continued = std::ranges::all_of(text.substr(1, length - 1), [](const char byte)
			{
				return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
			});
			return continued ? length : 1;
		}

		bool isOneCharacter(const std::string_view text)
		{
			return !text.empty() && characterLength(text) == text.size() &&
			       (text.size() > 1 || static_cast<unsigned char>(text[0]) < 0x80);
		}
	}

	CharMap CharMap::fromEntries(const std::span<const TextEntry> entries, std::vector<std::string>& problems)
	{
		CharMap map;
		for (const TextEntry& entry : entries)
		{
			if (isOneCharacter(entry.key))
				map.replacements_[entry.key] = entry.value;
			else
				problems.push_back(std::format("line {}: skipped, the key must be one character", entry.line));
		}

		return map;
	}

	CharMap CharMap::over(const CharMap& fallback) const
	{
		CharMap merged = *this;
		merged.replacements_.insert(fallback.replacements_.begin(), fallback.replacements_.end());
		return merged;
	}

	std::string CharMap::apply(std::string_view text) const
	{
		std::string result;
		result.reserve(text.size());
		while (!text.empty())
		{
			const std::string_view character = text.substr(0, characterLength(text));
			const auto replacement = replacements_.find(std::string(character));
			result += replacement != replacements_.end() ? std::string_view(replacement->second) : character;
			text.remove_prefix(character.size());
		}

		return result;
	}

	std::size_t CharMap::size() const noexcept
	{
		return replacements_.size();
	}
}
