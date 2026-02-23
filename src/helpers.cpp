#include "pch.hpp"
#include "helpers.hpp"

std::wstring GetDllDirectory(HMODULE hModule) {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(hModule, buf, MAX_PATH);
    std::wstring dir(buf);
    auto pos = dir.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? dir.substr(0, pos + 1) : dir;
}
