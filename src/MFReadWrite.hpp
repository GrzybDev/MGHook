#pragma once

#pragma comment(linker, "/EXPORT:MFCreateSourceReaderFromMediaSource=_Real_MFCreateSourceReaderFromMediaSource")

using fn_MFCreateSourceReaderFromMediaSource = HRESULT(WINAPI*)(void*, void*, void**);
static fn_MFCreateSourceReaderFromMediaSource real_MFCreateSourceReaderFromMediaSource = nullptr;

static HMODULE hRealDll = nullptr;

static void LoadRealDll()
{
	if (hRealDll) return;

	wchar_t sysDir[MAX_PATH];
	GetSystemDirectoryW(sysDir, MAX_PATH);
	lstrcatW(sysDir, L"\\MFReadWrite.dll");

	hRealDll = LoadLibraryW(sysDir);
	if (!hRealDll)
	{
		MessageBoxW(nullptr, L"Failed to load real MFReadWrite.dll", L"MGHook", MB_ICONERROR);
		return;
	}

	real_MFCreateSourceReaderFromMediaSource =
		(fn_MFCreateSourceReaderFromMediaSource)GetProcAddress(hRealDll, "MFCreateSourceReaderFromMediaSource");
}


extern "C" __declspec(dllexport)
HRESULT WINAPI _Real_MFCreateSourceReaderFromMediaSource(void* pMediaSource, void* pAttributes, void** ppSourceReader);
