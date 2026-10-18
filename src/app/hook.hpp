#pragma once

#include <windows.h>

namespace mghook
{
	/// Replaces the font of the game this library was loaded into, if it is the game.
	/// @param library This library, whose folder holds MGHook.ini and the font.
	void installHook(HMODULE library) noexcept;
}
