#include <gtest/gtest.h>

#include "text/char_map.hpp"

namespace mghook::test
{
	namespace
	{
		CharMap charMap(const std::string_view content, std::vector<std::string>* problems = nullptr)
		{
			const TextFile file = parseTextFile(content);
			std::vector<std::string> ignored;
			return CharMap::fromEntries(file.entries, problems ? *problems : ignored);
		}
	}

	TEST(CharMap, NeverMapsAReplacementAgain)
	{
		EXPECT_EQ(charMap("é=e\ne=X").apply("ée"), "eX");
	}

	TEST(CharMap, LetsARegionMapOverrideTheCommonOne)
	{
		const CharMap common = charMap("é=e\nü=u");
		const CharMap region = charMap("é=\\x5B");

		EXPECT_EQ(region.over(common).apply("éü"), "[u");
	}

	TEST(CharMap, PassesBytesThatAreNoCharacterThrough)
	{
		EXPECT_EQ(charMap("é=e").apply("\xFF\xC3" "é"), "\xFF\xC3" "e");
	}

	TEST(CharMap, SkipsKeysThatAreNotOneCharacter)
	{
		std::vector<std::string> problems;
		const CharMap map = charMap("ab=x\n\\xFF=y\nñ=n", &problems);

		EXPECT_EQ(problems.size(), 2u);
		EXPECT_EQ(map.size(), 1u);
		EXPECT_EQ(map.apply("abñ\xFF"), "abn\xFF");
	}
}
