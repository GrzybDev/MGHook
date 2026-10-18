#include "pch.hpp"
#include "hook.hpp"

#include "app/log.hpp"
#include "app/settings.hpp"
#include "font/font_replacement.hpp"
#include "game/image.hpp"
#include "text/regions.hpp"
#include "text/translation.hpp"
#include "version.hpp"

namespace mghook
{
	namespace
	{
		std::filesystem::path libraryFolder(const HMODULE library)
		{
			std::wstring path(MAX_PATH, L'\0');
			DWORD length;
			while ((length = GetModuleFileNameW(library, path.data(), static_cast<DWORD>(path.size()))) == path.size())
				path.resize(path.size() * 2);

			path.resize(length);
			return std::filesystem::path(path).parent_path();
		}

		void logProblems(const std::filesystem::path& file, const std::vector<std::string>& problems)
		{
			for (const std::string& problem : problems)
				log::warning("{}, {}", log::pathText(file.filename()), problem);
		}

		CharMap loadCharMap(const std::filesystem::path& file)
		{
			std::optional<TextFile> text = readTextFile(file);
			if (!text)
				return {};

			CharMap map = CharMap::fromEntries(text->entries, text->problems);
			logProblems(file, text->problems);
			log::info("Loaded {} character(s) from {}", text->entries.size(), log::pathText(file.filename()));
			return map;
		}

		void translateStrings(const ModuleImage& image, const std::filesystem::path& folder)
		{
			const std::filesystem::path file = folder / L"translations.txt";
			const std::optional<TextFile> translations = readTextFile(file);
			if (!translations)
			{
				log::info("No {}, so the strings stay as they are", log::pathText(file));
				return;
			}

			logProblems(file, translations->problems);
			if (image.timestamp() != kSupportedBuild)
			{
				log::error("This METAL GEAR.exe (built {:#x}) is not the one the string regions are known for ({:#x}), "
				           "so the strings stay as they are", image.timestamp(), kSupportedBuild);
				return;
			}

			const CharMap common = loadCharMap(folder / L"charmap.txt");
			std::vector<MappedRegion> regions;
			for (const StringRegion& region : kStringRegions)
			{
				const CharMap own = loadCharMap(folder / std::format("charmap_{}.txt", region.name));
				regions.push_back({.name = region.name, .bytes = image.at(region.rva, region.size), .charMap = own.over(common)});
			}

			const TextPlan plan = planTranslation(regions, translations->entries);
			for (const TextEntry& entry : plan.unmatched)
				log::warning("{}, line {}: no string in the regions reads \"{}\"", log::pathText(file.filename()),
				             entry.line, entry.key);
			for (const TextPatch& patch : plan.patches)
				log::info("{}: \"{}\" becomes \"{}\"{}", patch.region, patch.original, patch.translation,
				          patch.fitsInPlace() ? "" : ", which is longer than the original");

			const TextResult result = applyTranslation(plan.patches, image.relocatedPointers());
			for (const std::string& original : result.stranded)
				log::warning("\"{}\" stays as it is: its translation is longer and nothing points at it", original);

			log::info("Translated {} string(s) in place and moved {}, turning {} pointer(s)", result.inPlace,
			          result.moved, result.redirectedPointers);
		}

		template <typename Part>
		void runPart(const std::string_view name, Part part) noexcept
		{
			try
			{
				part();
			}
			catch (const std::exception& error)
			{
				log::error("{} failed: {}", name, error.what());
			}
		}
	}

	void installHook(const HMODULE library) noexcept
	{
		// The Unity launcher loads this library too; only the game, which runs on Engine.dll, gets patched.
		if (!GetModuleHandleW(L"Engine.dll"))
			return;

		runPart("The hook", [library]
		{
			const std::filesystem::path folder = libraryFolder(library);
			const Settings settings = Settings::load(folder / L"MGHook.ini");
			log::open({.console = settings.logToConsole, .file = settings.logToFile ? folder / L"MGHook.log" : std::filesystem::path()});
			log::info("MGHook {} by Marek Grzyb (@GrzybDev), https://github.com/GrzybDev/MGHook", kVersion);

			const ModuleImage image(GetModuleHandleW(nullptr));
			runPart("Replacing the font", [&]
			{
				replaceFont(image, folder / L"EngineSupport" / L"Fonts" / L"MGS_Font_nht.raw", settings.exportFont);
			});
			runPart("Translating the strings", [&] { translateStrings(image, folder); });
		});
	}
}
