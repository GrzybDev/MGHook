#pragma once

struct CharMapEntry
{
	std::string from; // UTF-8 bytes of source character
	std::string to; // UTF-8 bytes of replacement character
};

static std::vector<CharMapEntry> charMap;
static std::unordered_map<int, std::vector<CharMapEntry>> regionCharMaps;

void LoadCharSubstitutionMap(const std::wstring& path);
void LoadRegionCharSubstitutionMap(const std::wstring& path, int regionIndex);
std::string ApplyCharMap(const std::string& utf8Text, int regionIndex = -1);
