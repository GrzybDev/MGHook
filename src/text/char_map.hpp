#pragma once

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "text/text_file.hpp"

namespace mghook
{
	/// Replaces characters a font lacks with codes it has.
	class CharMap
	{
	public:
		/// Builds a map from the entries of a character map file.
		/// @param entries Entries whose key is one UTF-8 character and whose value is what replaces it.
		/// @param problems Gets a note on each entry whose key is not one character.
		static CharMap fromEntries(std::span<const TextEntry> entries, std::vector<std::string>& problems);

		/// Returns a map holding this map's entries and those of the fallback this map does not override.
		/// @param fallback The map to fall back on.
		[[nodiscard]] CharMap over(const CharMap& fallback) const;

		/// Replaces every mapped character of a text in one pass, so a replacement is never mapped again.
		/// @param text UTF-8 text, which may hold bytes that are no character.
		[[nodiscard]] std::string apply(std::string_view text) const;

	private:
		std::unordered_map<std::string, std::string> replacements_;
	};
}
