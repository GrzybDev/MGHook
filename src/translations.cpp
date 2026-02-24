#include "pch.hpp"
#include "translations.hpp"

std::vector<TranslationEntry> LoadTranslations(const std::wstring& path)
{
	std::vector<TranslationEntry> entries;
	std::ifstream file(path);

	if (!file.is_open())
		return entries;

	std::string line;
	bool firstLine = true;

	while (std::getline(file, line))
	{
		// Handle potential UTF-8 BOM on very first line
		if (firstLine)
		{
			firstLine = false;

			if (line.size() >= 3 &&
				line[0] == '\xEF' && line[1] == '\xBB' && line[2] == '\xBF')
			{
				line = line.substr(3);
			}
		}

		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;

		auto sep = line.find('=');
		if (sep == std::string::npos)
			continue;

		std::string original = Trim(line.substr(0, sep));
		std::string replacement = Trim(line.substr(sep + 1));

		if (original.empty() || replacement.empty())
			continue;

		entries.push_back({.original = std::move(original), .replacement = std::move(replacement)});
	}

	return entries;
}

TranslationResult ApplyTranslations(const std::vector<TranslationEntry>& translations, const AddressTuple& addresses)
{
	int patched = 0, skipped = 0, totalPtrs = 0;

	for (const auto& [original, replacement] : translations)
	{
		// Search for the original string (raw UTF-8 bytes) across all regions
		uintptr_t origAddr = 0;
		int foundRegion = -1;
		const char* matchKey = original.c_str();
		const size_t matchLen = original.length();

		for (int r = 0; r < NUM_STRING_REGIONS; ++r)
		{
			const uintptr_t rStart = addresses.baseAdr + STRING_REGIONS[r].rvaStart;
			const uintptr_t rEnd = addresses.baseAdr + STRING_REGIONS[r].rvaEnd;
			origAddr = FindString(rStart, rEnd, matchKey, matchLen);

			if (origAddr != 0)
			{
				foundRegion = r;
				break;
			}
		}

		if (origAddr == 0)
		{
			Logger::Log("[Translations] NOT FOUND: \"%s\"", original.c_str());
			++skipped;
			continue;
		}

		// Apply charmap substitutions to the replacement text (UTF-8)
		std::string replaceFinal = ApplyCharMap(replacement, foundRegion);
		const char* replaceData = replaceFinal.c_str();
		size_t replaceLen = replaceFinal.length();

		// Allocate a new persistent buffer for the translated string
		char* newStr = AllocatePersistentString(replaceData, replaceLen);

		if (!newStr)
		{
			Logger::Log("[Translations] ALLOC FAILED: \"%s\"", replacement.c_str());
			++skipped;
			continue;
		}

		const auto newVA = reinterpret_cast<uintptr_t>(newStr);

		// Scan .rdata and .data for pointer-table entries pointing to origAddr
		int ptrCount = 0;
		ptrCount += PatchPointersInRegion(addresses.rdataStart, RDATA_SIZE,
		                                  origAddr, newVA);
		ptrCount += PatchPointersInRegion(addresses.dataStart, DATA_SIZE,
		                                  origAddr, newVA);

		// Also patch the original string in-place (truncated) as fallback
		// for any direct references we might miss (e.g. RIP-relative LEA)
		const uintptr_t regionEnd = addresses.baseAdr + STRING_REGIONS[foundRegion].rvaEnd;
		size_t slotSize = GetSlotSize(origAddr, matchLen, regionEnd);
		const size_t inPlaceLen = (replaceLen <= slotSize) ? replaceLen : slotSize;
		std::memset(reinterpret_cast<void*>(origAddr), 0, slotSize + 1);
		std::memcpy(reinterpret_cast<void*>(origAddr), replaceData, inPlaceLen);

		// Ensure null terminator
		*reinterpret_cast<char*>(origAddr + inPlaceLen) = '\0';

		totalPtrs += ptrCount;
		++patched;

		Logger::Log(R"([Translations] PATCHED [%s]: "%s" -> "%s" (at 0x%llX, %d ptr(s), slot %zu))",
		            STRING_REGIONS[foundRegion].name,
		            original.c_str(), replacement.c_str(),
		            static_cast<unsigned long long>(origAddr), ptrCount, slotSize);
	}

	TranslationResult result;
	result.patchedCount = patched;
	result.skippedCount = skipped;
	result.totalPtrsPatched = totalPtrs;
	return result;
}
