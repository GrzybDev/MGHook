#include <cstring>

#include <gtest/gtest.h>
#include <windows.h>

#include "font/font_replacement.hpp"
#include "game/image.hpp"
#include "text/regions.hpp"
#include "text/translation.hpp"

namespace mghook::test
{
	namespace
	{
		const StringRegion& region(const std::string_view name)
		{
			return *std::ranges::find(kStringRegions, name, &StringRegion::name);
		}
	}

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

	TEST_F(GameTest, IsTheBuildTheRegionsWereTakenFrom)
	{
		EXPECT_EQ(ModuleImage(module_).timestamp(), kSupportedBuild);
	}

	TEST_F(GameTest, RegionsHoldWholeStringsTheGameReachesThroughPointers)
	{
		const ModuleImage image(module_);
		const std::pair<std::string_view, std::string_view> samples[] = {
			{"difficulty", "VERY EASY"}, {"difficulty", "EXTREMO EUROPEO"}, {"save_labels", "CLEAR DATA"},
			{"save_labels", "PARTIDA SUPERADA"}, {"menus", "DATA LOAD"}, {"menus", "PUNTO PARA CARGAR"},
			{"messages", "No data present on storage device."},
		};

		std::vector<std::uintptr_t> targets;
		for (std::uintptr_t* slot : image.relocatedPointers())
			targets.push_back(*slot);

		for (const auto& [name, text] : samples)
		{
			const StringRegion& known = region(name);
			const std::span<std::byte> bytes = image.at(known.rva, known.size);
			EXPECT_EQ(image.at(known.rva - 1, 1)[0], std::byte{0}) << name;
			EXPECT_EQ(bytes.back(), std::byte{0}) << name;

			const MappedRegion mappedRegion{.name = name, .bytes = bytes, .charMap = {}};
			const TextEntry entry{.key = std::string(text), .value = "X", .line = 1};
			const TextPlan plan = planTranslation(std::span(&mappedRegion, 1), std::span(&entry, 1));
			ASSERT_EQ(plan.patches.size(), 1u) << text;
			EXPECT_NE(std::ranges::find(targets, reinterpret_cast<std::uintptr_t>(plan.patches[0].room.data())),
			          targets.end()) << text;
		}
	}

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
