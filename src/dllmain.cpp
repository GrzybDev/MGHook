// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.hpp"

extern "C" __declspec(dllexport)
HRESULT WINAPI _Real_MFCreateSourceReaderFromMediaSource(void* pMediaSource, void* pAttributes, void** ppSourceReader)
{
	if (!real_MFCreateSourceReaderFromMediaSource)
		LoadRealDll();

	if (!real_MFCreateSourceReaderFromMediaSource)
		return E_FAIL;

	return real_MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, ppSourceReader);
}

void PrintMotd()
{
	printf("Metal Gear 1 / 2 hook %s by Marek Grzyb (@GrzybDev)\n", VERSION);
	printf("Homepage: https://grzyb.dev/project/MGHook\n");
	printf("Source code: https://github.com/GrzybDev/MGHook\n");
	printf("Noticed a bug? Fill a bug report here: https://github.com/GrzybDev/MGHook/issues\n");
	printf("Licensed under GNU Lesser General Public License v3, Contributions of any kind welcome!");
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);

		DebugConsole();
		PrintMotd();

		LoadRealDll();
	}

	return TRUE;
}
