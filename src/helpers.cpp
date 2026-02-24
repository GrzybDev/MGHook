#include "pch.hpp"
#include "helpers.hpp"

std::wstring GetDllDirectory(const HMODULE hModule)
{
	wchar_t buf[MAX_PATH]{};
	GetModuleFileNameW(hModule, buf, MAX_PATH);
	std::wstring dir(buf);
	const auto pos = dir.find_last_of(L"\\/");
	return (pos != std::wstring::npos) ? dir.substr(0, pos + 1) : dir;
}

std::string Trim(const std::string& s)
{
	const auto start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return {};
	const auto end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

std::wstring Utf8ToWide(const std::string& utf8)
{
	if (utf8.empty())
		return {};

	const int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
	                                     static_cast<int>(utf8.size()), nullptr, 0);

	if (wlen <= 0)
		return {};

	std::wstring wide(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
	                    static_cast<int>(utf8.size()), wide.data(), wlen);

	return wide;
}

std::string WideToUtf8(const std::wstring& wide)
{
	if (wide.empty())
		return {};

	const int ulen = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
	                                     static_cast<int>(wide.size()),
	                                     nullptr, 0, nullptr, nullptr);

	if (ulen <= 0)
		return {};

	std::string utf8(ulen, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
	                    static_cast<int>(wide.size()),
	                    utf8.data(), ulen, nullptr, nullptr);

	return utf8;
}

uintptr_t FindString(const uintptr_t regionStart, const uintptr_t regionEnd,
                     const char* needle, const size_t needleLen)
{
	if (needleLen == 0)
		return 0;

	if (regionStart + needleLen + 1 > regionEnd)
		return 0;

	const uintptr_t limit = regionEnd - needleLen;

	for (uintptr_t addr = regionStart; addr < limit; ++addr)
	{
		if (std::memcmp(reinterpret_cast<const void*>(addr), needle,
		                needleLen + 1) == 0)
		{
			return addr;
		}
	}

	return 0;
}

char* AllocatePersistentString(const char* text, const size_t len)
{
	size_t allocSize = len + 1;

	allocSize = std::max<size_t>(allocSize, 4096);

	void* mem = VirtualAlloc(nullptr, allocSize,
	                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!mem)
		return nullptr;

	std::memcpy(mem, text, len + 1);

	// Make it read-only (matches the original .rdata protection)
	DWORD oldProt;
	VirtualProtect(mem, allocSize, PAGE_READONLY, &oldProt);

	allocatedStrings.push_back(static_cast<char*>(mem));
	return static_cast<char*>(mem);
}

int PatchPointersInRegion(const uintptr_t scanStart, const size_t scanSize, const uintptr_t oldVA,
                          const uintptr_t newVA)
{
	int count = 0;
	const uintptr_t scanEnd = scanStart + scanSize - sizeof(uintptr_t);

	for (uintptr_t addr = scanStart; addr <= scanEnd;
	     addr += sizeof(uintptr_t))
	{
		if (const uintptr_t val = *reinterpret_cast<uintptr_t*>(addr); val == oldVA)
		{
			*reinterpret_cast<uintptr_t*>(addr) = newVA;
			++count;
		}
	}

	return count;
}

size_t GetSlotSize(const uintptr_t addr, const size_t strLen, const uintptr_t regionEnd)
{
	uintptr_t scan = addr + strLen + 1;

	while (scan < regionEnd && *reinterpret_cast<const char*>(scan) == '\0')
		++scan;

	return scan - addr - 1;
}


size_t GetGzipStreamSize(const void* data, const size_t maxSize)
{
	z_stream strm{};
	strm.next_in = static_cast<Bytef*>(const_cast<void*>(data));
	strm.avail_in = static_cast<uInt>((std::min)(maxSize, static_cast<size_t>(UINT_MAX)));

	if (inflateInit2(&strm, 15 + 16) != Z_OK) // 15+16 = gzip auto
		return 0;

	Bytef discard[32768];
	int ret;
	do
	{
		strm.next_out = discard;
		strm.avail_out = sizeof(discard);
		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
		{
			inflateEnd(&strm);
			return 0;
		}
	}
	while (ret != Z_STREAM_END);

	const size_t consumed = maxSize - strm.avail_in;
	inflateEnd(&strm);
	return consumed;
}
