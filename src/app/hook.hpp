#pragma once

#include <windows.h>

namespace mghook
{
	/// Prepares the game this library was loaded into, if it is the game.
	/// @param library This library, whose folder holds MGHook.ini.
	void installHook(HMODULE library) noexcept;
}
