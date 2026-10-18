#include <cstring>

#include <gtest/gtest.h>

#include "text/translation.hpp"

namespace mghook::test
{
	namespace
	{
		// Laid out like the game's regions: each string padded with zeros to the next multiple of four bytes.
		constexpr char kLayout[] = "OK\0\0SAVE\0\0\0\0DATA LOAD\0\0\0ORPHAN\0\0OK\0\0";

		std::string_view at(const std::span<const std::byte> bytes, const std::size_t offset)
		{
			return reinterpret_cast<const char*>(bytes.data() + offset);
		}
	}

	// A region in plain memory with a pointer table, the way the game reaches its strings.
	class TranslationTest : public testing::Test
	{
	protected:
		TranslationTest() : memory_(sizeof(kLayout) - 1)
		{
			std::memcpy(memory_.data(), kLayout, memory_.size());
			table_ = {address(0), address(4), address(4)};
			for (std::uintptr_t& entry : table_)
				pointers_.push_back(&entry);
		}

		std::uintptr_t address(const std::size_t offset)
		{
			return reinterpret_cast<std::uintptr_t>(memory_.data() + offset);
		}

		TextResult translate(const std::string_view translations, const std::string_view charMap = {})
		{
			std::vector<std::string> problems;
			const MappedRegion region{.name = "menus", .bytes = memory_,
			                          .charMap = CharMap::fromEntries(parseTextFile(charMap).entries, problems)};
			plan_ = planTranslation(std::span(&region, 1), parseTextFile(translations).entries);
			return applyTranslation(plan_.patches, pointers_);
		}

		std::vector<std::byte> memory_;
		std::vector<std::uintptr_t> table_;
		std::vector<std::uintptr_t*> pointers_;
		TextPlan plan_;
	};

	TEST_F(TranslationTest, WritesATranslationThatFitsOverEveryCopyOfItsOriginal)
	{
		const TextResult result = translate("OK=YES\nDATA LOAD=READ DATA");

		EXPECT_EQ(result.inPlace, 3u);
		EXPECT_EQ(at(memory_, 0), "YES");
		EXPECT_EQ(at(memory_, 12), "READ DATA");
		EXPECT_EQ(at(memory_, 32), "YES");
		EXPECT_EQ(table_[0], address(0));
	}

	TEST_F(TranslationTest, MovesALongerTranslationAndTurnsEveryPointerToIt)
	{
		const TextResult result = translate("SAVE=SAVE THE GAME");

		EXPECT_EQ(result.moved, 1u);
		EXPECT_EQ(result.redirectedPointers, 2u);
		EXPECT_EQ(reinterpret_cast<const char*>(table_[1]), std::string_view("SAVE THE GAME"));
		EXPECT_EQ(table_[2], table_[1]);
		EXPECT_EQ(at(memory_, 4), "SAVE");
	}

	TEST_F(TranslationTest, EncodesTranslationsWithTheRegionsCharacterMap)
	{
		translate("SAVE=SAVE THE GAMÉ", "É=E");

		EXPECT_EQ(reinterpret_cast<const char*>(table_[1]), std::string_view("SAVE THE GAME"));
	}

	TEST_F(TranslationTest, MatchesWholeStringsOnly)
	{
		const TextResult result = translate("AVE=X\nOK =X\nMISSING=X\nOK=YES");

		ASSERT_EQ(plan_.unmatched.size(), 3u);
		EXPECT_EQ(plan_.unmatched[0].key, "AVE");
		EXPECT_EQ(plan_.unmatched[1].key, "OK ");
		EXPECT_EQ(plan_.unmatched[2].key, "MISSING");
		EXPECT_EQ(result.inPlace, 2u);
	}

	TEST_F(TranslationTest, LeavesALongerTranslationNothingPointsAtUntranslated)
	{
		const TextResult result = translate("ORPHAN=A LONGER ORPHAN");

		EXPECT_EQ(result.moved, 0u);
		ASSERT_EQ(result.stranded.size(), 1u);
		EXPECT_EQ(result.stranded[0], "ORPHAN");
		EXPECT_EQ(at(memory_, 24), "ORPHAN");
	}
}
