#pragma once

void LoadCharSubstitutionMap(const std::wstring& path);
void LoadRegionCharSubstitutionMap(const std::wstring& path, int regionIndex);
std::string ApplyCharMap(const std::string& utf8Text, int regionIndex = -1);
