#include "pch.hpp"
#include "hook.hpp"

#include "app/log.hpp"
#include "app/settings.hpp"
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
		});
	}
}
