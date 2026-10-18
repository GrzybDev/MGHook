#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "text/char_map.hpp"
#include "text/text_file.hpp"

namespace mghook
{
	/// A string region as the game has it in memory, with the character map of the font it is drawn with.
	struct MappedRegion
	{
		std::string_view name;
		std::span<std::byte> bytes;
		CharMap charMap;
	};

	/// A string of a region and the translation that replaces it.
	struct TextPatch
	{
		std::string_view region;
		/// The string with its terminator and the zero padding up to the next string, all of which a translation may fill.
		std::span<std::byte> room;
		std::string original;
		/// The translation with the region's character map applied.
		std::string translation;

		/// Tells whether the translation and its terminator fit where the original is.
		[[nodiscard]] bool fitsInPlace() const
		{
			return translation.size() < room.size();
		}
	};

	/// The strings translations replace, and the translations whose original no region holds.
	struct TextPlan
	{
		std::vector<TextPatch> patches;
		std::vector<TextEntry> unmatched;
	};

	/// What writing the patches did.
	struct TextResult
	{
		std::size_t inPlace = 0;
		std::size_t moved = 0;
		std::size_t redirectedPointers = 0;
		/// Originals too short for their translation that nothing points at, so they stay as they are.
		std::vector<std::string> stranded;
	};

	/// Matches every string of the regions against the originals of the translations.
	/// @param regions The regions to search.
	/// @param translations Entries mapping an original string, byte for byte, to its translation.
	[[nodiscard]] TextPlan planTranslation(std::span<const MappedRegion> regions, std::span<const TextEntry> translations);

	/// Writes each translation over its original when it fits, otherwise into new memory that every pointer to the
	/// original is turned to; the original then stays for anything the pointers do not cover.
	/// @param patches The patches of a plan.
	/// @param pointers Every slot holding an absolute address in the image, which the strings are reached through.
	TextResult applyTranslation(std::span<const TextPatch> patches, std::span<std::uintptr_t* const> pointers);
}
