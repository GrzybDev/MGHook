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
	Logger::Log("Metal Gear 1 / 2 hook v%s by Marek Grzyb (@GrzybDev)\n", VERSION);
	Logger::Log("Homepage: https://grzyb.dev/project/MGHook\n");
	Logger::Log("Source code: https://github.com/GrzybDev/MGHook\n");
	Logger::Log("Noticed a bug? Fill a bug report here: https://github.com/GrzybDev/MGHook/issues\n");
	Logger::Log("Licensed under GNU Lesser General Public License v3, Contributions of any kind welcome!");
	Logger::Log("\n\n");
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);

		Logger::Init(hModule);
		PrintMotd();

		LoadRealDll();
	}

	return TRUE;
}
