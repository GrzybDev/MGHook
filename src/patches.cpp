#include "pch.hpp"
#include "patches.hpp"

void PatchFont(HMODULE hModule)
{
	HMODULE exeBase = GetModuleHandleW(nullptr);

	if (!exeBase)
	{
		Logger::Log("[Font] ERROR: GetModuleHandleW(NULL) failed in font handler.");
		return;
	}

	FontParameters fontParams;

	const auto base = reinterpret_cast<uintptr_t>(exeBase);
	fontParams.gzipPtr = reinterpret_cast<const char*>(base + FONT_GZIP_RVA);
	fontParams.baseAddr = base;

	// Validate gzip magic bytes
	if (static_cast<uint8_t>(fontParams.gzipPtr[0]) != 0x1F ||
		static_cast<uint8_t>(fontParams.gzipPtr[1]) != 0x8B)
	{
		Logger::Log("[Font] ERROR: No gzip magic at RVA 0x%X (found 0x%02X 0x%02X).",
		            FONT_GZIP_RVA,
		            static_cast<unsigned>(static_cast<uint8_t>(fontParams.gzipPtr[0])),
		            static_cast<unsigned>(static_cast<uint8_t>(fontParams.gzipPtr[1])));
		return;
	}

	// Measure the exact compressed-stream size
	fontParams.origGzipSize = GetGzipStreamSize(fontParams.gzipPtr, FONT_GZIP_MAX_SIZE);
	if (fontParams.origGzipSize == 0)
	{
		Logger::Log("[Font] ERROR: Could not determine gzip stream size.");
		return;
	}

	Logger::Log("[Font] Font gzip at RVA 0x%X, compressed size: %zu bytes.",
	            FONT_GZIP_RVA, fontParams.origGzipSize);

	// Build paths
	const std::wstring dllDir = GetDllDirectory(hModule);
	fontParams.engineDir = dllDir + L"EngineSupport";
	fontParams.fontsDir = fontParams.engineDir + L"\\Fonts";
	fontParams.fontPath = fontParams.fontsDir + L"\\MGS_Font_nht.raw";

	const bool fontFileExists =
		(GetFileAttributesW(fontParams.fontPath.c_str()) != INVALID_FILE_ATTRIBUTES);

	if (!fontFileExists)
	{
		Logger::Log("[Font] Font file not found at path: %s", WideToUtf8(fontParams.fontPath).c_str());
		ExtractFont(fontParams);
	}
	else
	{
		Logger::Log("[Font] Font file found at path: %s", WideToUtf8(fontParams.fontPath).c_str());
		ReplaceFont(fontParams);
	}
}

void PatchStrings(const HMODULE hModule)
{
	// Resolve base address
	HMODULE exeBase = GetModuleHandleW(nullptr);

	if (!exeBase)
	{
		Logger::Log("[Translations] ERROR: GetModuleHandleW failed.");
		return;
	}

	AddressTuple addresses;
	addresses.baseAdr = reinterpret_cast<uintptr_t>(exeBase);
	addresses.rdataStart = addresses.baseAdr + RDATA_RVA;
	addresses.dataStart = addresses.baseAdr + DATA_RVA;

	Logger::Log("[Translations] Base address: 0x%p", exeBase);

	for (auto [rvaStart, rvaEnd, name] : STRING_REGIONS)
	{
		const auto rs = addresses.baseAdr + rvaStart;
		const auto re = addresses.baseAdr + rvaEnd;
		Logger::Log("[Translations]  %s : 0x%llX - 0x%llX (%u bytes)",
		            name,
		            static_cast<unsigned long long>(rs), static_cast<unsigned long long>(re),
		            rvaEnd - rvaStart);
	}

	// Load translations
	const std::wstring dllDir = GetDllDirectory(hModule);
	LoadCharSubstitutionMap(dllDir + L"charmap.txt");
	const auto translations = LoadTranslations(dllDir + L"translations.txt");

	if (translations.empty())
	{
		Logger::Log(
			"[Translations] No translations loaded (file missing or doesn't contain translations). (Tried path: %s)",
			WideToUtf8(dllDir + L"translations.txt").c_str());
		return;
	}

	Logger::Log("[Translations] Loaded %zu translation(s).", translations.size());

	// Make the string regions + pointer table regions writable
	DWORD oldProtRdata = 0, oldProtData = 0;

	if (!VirtualProtect(reinterpret_cast<void*>(addresses.rdataStart),
	                    RDATA_SIZE, PAGE_READWRITE, &oldProtRdata))
	{
		Logger::Log("[Translations] ERROR: VirtualProtect(.rdata) failed (err %lu).", GetLastError());
		return;
	}

	if (!VirtualProtect(reinterpret_cast<void*>(addresses.dataStart),
	                    DATA_SIZE, PAGE_READWRITE, &oldProtData))
	{
		Logger::Log("[Translations] WARN: VirtualProtect(.data) failed (err %lu), "
		            "some pointers may not be patched.", GetLastError());
	}

	auto [patchedCount, skippedCount, totalPtrsPatched] = ApplyTranslations(translations, addresses);

	// Restore memory protection
	VirtualProtect(reinterpret_cast<void*>(addresses.rdataStart),
	               RDATA_SIZE, oldProtRdata, &oldProtRdata);
	VirtualProtect(reinterpret_cast<void*>(addresses.dataStart),
	               DATA_SIZE, oldProtData, &oldProtData);

	Logger::Log("[Translations] Done. Patched: %d  Skipped: %d  Pointers redirected: %d",
	            patchedCount, skippedCount, totalPtrsPatched);
}
