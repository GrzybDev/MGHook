#include "pch.hpp"

#include "app/hook.hpp"

BOOL APIENTRY DllMain(const HMODULE module, const DWORD reason, LPVOID /*reserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(module);

		// The game unpacks its font right after its entry point runs, which is after every library it imports has
		// been initialized, so patching here is in time; nothing below loads another library under the loader lock.
		mghook::installHook(module);
	}

	return TRUE;
}
