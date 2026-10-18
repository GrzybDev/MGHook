#include "pch.hpp"
#include "settings.hpp"

namespace mghook
{
	namespace
	{
		bool readSwitch(const std::filesystem::path& file, const wchar_t* section, const wchar_t* key)
		{
			return GetPrivateProfileIntW(section, key, 0, file.c_str()) != 0;
		}
	}

	Settings Settings::load(const std::filesystem::path& file)
	{
		return {
			.logToConsole = readSwitch(file, L"Log", L"Console"),
			.logToFile = readSwitch(file, L"Log", L"File"),
		};
	}
}
