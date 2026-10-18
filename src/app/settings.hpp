#pragma once

#include <filesystem>

namespace mghook
{
	/// The switches of MGHook.ini; each is off unless the file turns it on.
	struct Settings
	{
		bool logToConsole = false;
		bool logToFile = false;
		bool exportFont = false;

		/// Reads the switches, keeping the default of each one the file does not set.
		/// @param file The configuration file, which may be missing.
		static Settings load(const std::filesystem::path& file);
	};
}
