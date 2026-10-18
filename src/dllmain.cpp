#include "pch.hpp"

#include "app/hook.hpp"

BOOL APIENTRY DllMain(const HMODULE module, const DWORD reason, LPVOID /*reserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(module);

		mghook::installHook(module);
	}

	return TRUE;
}
