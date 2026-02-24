#include "pch.hpp"
#include "patches.hpp"

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
