#include <gtest/gtest.h>

#include "text/text_file.hpp"

namespace mghook::test
{
	TEST(TextFile, KeepsKeysAndValuesExactlyAsWritten)
	{
		const TextFile file = parseTextFile("\xEF\xBB\xBF# comment\r\n\r\n; comment\r\nAPPROVED BY =SIGNED BY \r\nOK=FINE");

		ASSERT_EQ(file.entries.size(), 2u);
		EXPECT_EQ(file.entries[0].key, "APPROVED BY ");
		EXPECT_EQ(file.entries[0].value, "SIGNED BY ");
		EXPECT_EQ(file.entries[0].line, 4);
		EXPECT_EQ(file.entries[1].key, "OK");
		EXPECT_EQ(file.entries[1].value, "FINE");
		EXPECT_TRUE(file.problems.empty());
	}

	TEST(TextFile, DecodesEscapesOnBothSides)
	{
		const TextFile file = parseTextFile(R"(TWO\nLINES\=EQUAL=A\tB\\C\=D\x5B\xc3\xa9)");

		ASSERT_EQ(file.entries.size(), 1u);
		EXPECT_EQ(file.entries[0].key, "TWO\nLINES=EQUAL");
		EXPECT_EQ(file.entries[0].value, "A\tB\\C=D[\xC3\xA9");
	}

	TEST(TextFile, SkipsBrokenLinesAndSaysWhichOnes)
	{
		const TextFile file = parseTextFile("NO SEPARATOR\n=EMPTY KEY\nEMPTY VALUE=\nBAD\\q=X\nSHORT=\\x4\nOK=FINE\nEND=\\");

		ASSERT_EQ(file.entries.size(), 1u);
		EXPECT_EQ(file.entries[0].key, "OK");
		ASSERT_EQ(file.problems.size(), 6u);
		for (std::size_t index = 0; const int line : {1, 2, 3, 4, 5, 7})
			EXPECT_TRUE(file.problems[index++].starts_with(std::format("line {}: skipped", line)));
	}

	TEST(TextFile, LetsALaterLineReplaceTheSameKey)
	{
		const TextFile file = parseTextFile("SAVE=STORE\nLOAD=READ\nSAVE=KEEP");

		ASSERT_EQ(file.entries.size(), 2u);
		EXPECT_EQ(file.entries[0].value, "KEEP");
		EXPECT_EQ(file.entries[0].line, 3);
		ASSERT_EQ(file.problems.size(), 1u);
		EXPECT_EQ(file.problems[0], "line 3: replaces the same key on line 1");
	}
}
