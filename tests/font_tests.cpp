#include <string>

#include <gtest/gtest.h>
#include <zlib.h>

#include "font/font_replacement.hpp"
#include "font/gzip.hpp"

namespace mghook::test
{
	namespace
	{
		constexpr int kEngineResult = 42;
		int gEngineCalls = 0;

		int engineUncompress(unsigned char*, unsigned int*, unsigned char*, unsigned int)
		{
			++gEngineCalls;
			return kEngineResult;
		}

		std::string gzipMember(const std::string& content, const char* name)
		{
			z_stream stream{};
			deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
			gz_header header{};
			header.name = reinterpret_cast<Bytef*>(const_cast<char*>(name));
			deflateSetHeader(&stream, &header);

			std::string member(deflateBound(&stream, static_cast<uLong>(content.size())) + 64, '\0');
			stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(content.data()));
			stream.avail_in = static_cast<uInt>(content.size());
			stream.next_out = reinterpret_cast<Bytef*>(member.data());
			stream.avail_out = static_cast<uInt>(member.size());
			deflate(&stream, Z_FINISH);
			member.resize(stream.total_out);
			deflateEnd(&stream);
			return member;
		}
	}

	// A .data section holding the built-in font between unrelated bytes, and a replacement larger than it.
	class FontTest : public testing::Test
	{
	protected:
		FontTest() : section_(kLeadingData + gzipMember(builtIn_, "MGS_Font_nht.raw") + kTrailingData)
		{
			gEngineCalls = 0;
		}

		std::span<std::byte> section()
		{
			return std::as_writable_bytes(std::span(section_));
		}

		int uncompress(const FontSubstitute& substitute, const BuiltInFont& font, std::string& buffer)
		{
			unsigned int size = static_cast<unsigned int>(buffer.size());
			const int result = substitute.uncompress(reinterpret_cast<unsigned char*>(buffer.data()), &size,
			                                         reinterpret_cast<unsigned char*>(font.member.data()),
			                                         static_cast<unsigned int>(font.member.size()));
			buffer.resize(size);
			return result;
		}

		// The name alone, with no gzip header before it, must not pass for the font.
		inline static const std::string kLeadingData = std::string("a string names MGS_Font_nht.raw\0 first", 38);
		inline static const std::string kTrailingData = "trailing data";

		std::string builtIn_ = std::string(5000, 'g') + "TNOF built-in";
		std::string replacement_ = "TNOF replacement, longer than the built-in font: " + std::string(6000, 'r');
		std::string section_;
	};

	TEST_F(FontTest, FindsTheGzipMemberNamedAfterTheFont)
	{
		const std::optional<BuiltInFont> font = findBuiltInFont(section());

		ASSERT_TRUE(font);
		EXPECT_EQ(font->content, builtIn_);
		EXPECT_EQ(font->member.data(), section().data() + kLeadingData.size());
		EXPECT_EQ(font->member.size(), section_.size() - kLeadingData.size() - kTrailingData.size());
	}

	TEST_F(FontTest, IgnoresMembersWithAnotherName)
	{
		section_ = gzipMember(builtIn_, "MGS_Font.raw");

		EXPECT_FALSE(findBuiltInFont(section()));
	}

	TEST_F(FontTest, RejectsAMemberCutShort)
	{
		section_.resize(kLeadingData.size() + 40);

		EXPECT_THROW((void)findBuiltInFont(section()), std::runtime_error);
	}

	TEST_F(FontTest, HandsTheEngineTheReplacementInTheRoomItMadeForIt)
	{
		const BuiltInFont font = *findBuiltInFont(section());
		const FontSubstitute substitute(font.member, replacement_, &engineUncompress);
		substitute.announceSize();
		ASSERT_EQ(gzipSizeField(font.member), replacement_.size());

		std::string buffer(gzipSizeField(font.member), '\0');

		EXPECT_EQ(uncompress(substitute, font, buffer), Z_OK);
		EXPECT_EQ(buffer, replacement_);
		EXPECT_EQ(gEngineCalls, 0);
	}

	TEST_F(FontTest, LeavesOtherStreamsToTheEngine)
	{
		const BuiltInFont font = *findBuiltInFont(section());
		const FontSubstitute substitute(font.member, replacement_, &engineUncompress);

		unsigned int size = 0;
		unsigned char other = 0;
		EXPECT_EQ(substitute.uncompress(nullptr, &size, &other, 1), kEngineResult);
		EXPECT_EQ(gEngineCalls, 1);
	}

	TEST_F(FontTest, KeepsTheBuiltInFontWhenTheEngineMadeTooLittleRoom)
	{
		const BuiltInFont font = *findBuiltInFont(section());
		const FontSubstitute substitute(font.member, replacement_, &engineUncompress);
		substitute.announceSize();

		std::string buffer(builtIn_.size(), '\0');

		EXPECT_EQ(uncompress(substitute, font, buffer), kEngineResult);
		EXPECT_EQ(gzipSizeField(font.member), builtIn_.size());
		EXPECT_EQ(gEngineCalls, 1);
	}
}
