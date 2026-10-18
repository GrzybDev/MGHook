#pragma once

#include <windows.h>

namespace mghook
{
	/// Replaces the font and translates the strings of the game this library was loaded into, if it is the game.
	/// @param library This library, whose folder holds MGHook.ini, the font and the translations.
	void installHook(HMODULE library) noexcept;
}
