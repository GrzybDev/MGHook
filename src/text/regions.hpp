#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace mghook
{
	/// A block of NUL-terminated strings in the executable that the game shows as they are.
	struct StringRegion
	{
		/// Names the region in the log and in its character map file, charmap_<name>.txt.
		std::string_view name;
		std::uint32_t rva;
		std::uint32_t size;
	};

	/// The header timestamp of METAL GEAR.exe 2.0.2.0, the build the regions were taken from.
	inline constexpr std::uint32_t kSupportedBuild = 0x68EC6F77;

	/// The string regions of that build: the first three come from MGS3, whose 8-bit text uses lowercase letters and
	/// some symbols for accented capitals; the messages are UTF-8 and drawn with the TNOF font.
	inline constexpr std::array kStringRegions{
		StringRegion{.name = "difficulty", .rva = 0x14A6E8, .size = 0x130},
		StringRegion{.name = "save_labels", .rva = 0x14D248, .size = 0x61},
		StringRegion{.name = "menus", .rva = 0x1502A0, .size = 0x50CA},
		StringRegion{.name = "messages", .rva = 0x155E40, .size = 0x124F},
	};
}
