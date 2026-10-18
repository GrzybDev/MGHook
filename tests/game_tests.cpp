#include <cstring>

#include <gtest/gtest.h>
#include <windows.h>

#include "font/font_replacement.hpp"
#include "game/image.hpp"

namespace mghook::test
{
	// Checks what the hook knows about METAL GEAR.exe against the real file, named by MGHOOK_GAME_EXE.
	class GameTest : public testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			path_.resize(GetEnvironmentVariableW(L"MGHOOK_GAME_EXE", nullptr, 0));
			if (path_.empty())
				return;

			path_.resize(GetEnvironmentVariableW(L"MGHOOK_GAME_EXE", path_.data(), static_cast<DWORD>(path_.size())));
			// The loader maps and relocates the image without running any of it or loading what it imports.
			module_ = LoadLibraryExW(path_.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
		}

		static void TearDownTestSuite()
		{
			if (module_)
				FreeLibrary(module_);
		}

		void SetUp() override
		{
			if (path_.empty())
				GTEST_SKIP() << "Set MGHOOK_GAME_EXE to METAL GEAR.exe to check the hook against the game";
			ASSERT_NE(module_, nullptr) << "MGHOOK_GAME_EXE names no executable that can be loaded";
		}

		inline static std::wstring path_;
		inline static HMODULE module_ = nullptr;
	};

	TEST_F(GameTest, CarriesItsFontAsAGzippedTnofFont)
	{
		const ModuleImage image(module_);
		const std::optional<BuiltInFont> font = findBuiltInFont(image.section(".data"));

		ASSERT_TRUE(font);
		EXPECT_TRUE(font->content.starts_with("TNOF"));
		std::uint32_t sizeField;
		std::memcpy(&sizeField, font->member.last(sizeof(sizeField)).data(), sizeof(sizeField));
		EXPECT_EQ(sizeField, font->content.size());
	}

	TEST_F(GameTest, ImportsUncompressGzipFromEngine)
	{
		EXPECT_NE(ModuleImage(module_).importSlot("Engine.dll", kUncompressGzipImport), nullptr);
	}
}
