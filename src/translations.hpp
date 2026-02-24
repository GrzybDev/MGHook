#pragma once

static std::vector<char*> allocatedStrings;

struct TranslationEntry
{
	std::string original; // key as raw UTF-8 bytes
	std::string replacement; // value as raw UTF-8 bytes
};

struct TranslationResult
{
	int patchedCount;
	int skippedCount;
	int totalPtrsPatched;
};

struct AddressTuple
{
	uintptr_t rdataStart;
	uintptr_t dataStart;
	uintptr_t baseAdr;
};

std::vector<TranslationEntry> LoadTranslations(const std::wstring& path);
TranslationResult ApplyTranslations(const std::vector<TranslationEntry>& translations, const AddressTuple& addresses);
