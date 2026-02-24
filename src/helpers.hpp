#pragma once

std::wstring GetDllDirectory(HMODULE hModule);
std::string Trim(const std::string& s);

std::wstring Utf8ToWide(const std::string& utf8);
std::string WideToUtf8(const std::wstring& wide);

uintptr_t FindString(uintptr_t regionStart, uintptr_t regionEnd,
                     const char* needle, size_t needleLen);

char* AllocatePersistentString(const char* text, size_t len);
int PatchPointersInRegion(uintptr_t scanStart, size_t scanSize, uintptr_t oldVA, uintptr_t newVA);
size_t GetSlotSize(uintptr_t addr, size_t strLen, uintptr_t regionEnd);

size_t GetGzipStreamSize(const void* data, size_t maxSize);
