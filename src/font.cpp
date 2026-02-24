#include "pch.hpp"
#include "font.hpp"

void ExtractFont(const FontParameters& fontParams)
{
	Logger::Log("[Font] Extracting original font from executable...");

	std::string decompressed;
	try
	{
		decompressed = gzip::decompress(fontParams.gzipPtr, fontParams.origGzipSize);
	}
	catch (const std::exception& e)
	{
		Logger::Log("[Font] ERROR: gzip decompress failed: %s", e.what());
		return;
	}

	// Create directory tree
	CreateDirectoryW(fontParams.engineDir.c_str(), nullptr);
	CreateDirectoryW(fontParams.fontsDir.c_str(), nullptr);

	const HANDLE hFile = CreateFileW(fontParams.fontPath.c_str(), GENERIC_WRITE, 0,
	                                 nullptr, CREATE_NEW,
	                                 FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::Log("[Font] ERROR: Cannot create font file (err %lu).", GetLastError());
		return;
	}

	DWORD written = 0;
	WriteFile(hFile, decompressed.data(),
	          static_cast<DWORD>(decompressed.size()), &written, nullptr);
	CloseHandle(hFile);

	Logger::Log("[Font] Extracted font -> EngineSupport\\Fonts\\MGS_Font_nht.raw "
	            "(%lu bytes written).", written);
}

void ReplaceFont(const FontParameters& fontParams)
{
	Logger::Log("[Font] Font file found on disk, loading...");

	const HANDLE hFile = CreateFileW(fontParams.fontPath.c_str(), GENERIC_READ,
	                                 FILE_SHARE_READ, nullptr,
	                                 OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Logger::Log("[Font] ERROR: Cannot open font file (err %lu).", GetLastError());
		return;
	}

	LARGE_INTEGER liSize;
	GetFileSizeEx(hFile, &liSize);
	std::vector<char> fontData(static_cast<size_t>(liSize.QuadPart));
	DWORD bytesRead = 0;
	ReadFile(hFile, fontData.data(),
	         static_cast<DWORD>(fontData.size()), &bytesRead, nullptr);
	CloseHandle(hFile);
	Logger::Log("[Font] Loaded font from disk: %lu bytes.", bytesRead);

	// Recompress with gzip (best compression)
	std::string compressed;
	try
	{
		compressed = gzip::compress(fontData.data(), fontData.size(),
		                            Z_BEST_COMPRESSION);
	}
	catch (const std::exception& e)
	{
		Logger::Log("[Font] ERROR: gzip compress failed: %s", e.what());
		return;
	}
	Logger::Log("[Font] Recompressed font: %zu bytes (original: %zu bytes).",
	            compressed.size(), fontParams.origGzipSize);

	if (const size_t origSize = fontParams.origGzipSize; compressed.size() <= origSize)
	{
		// Build a gzip stream padded to exactly origSize by inserting an
		// FCOMMENT field into the header (RFC 1952).  This keeps the total
		// byte count identical to the original so the game's decompressor
		// consumes every byte and doesn't choke on trailing zeros.
		const size_t padNeeded = origSize - compressed.size();

		std::vector<char> padded;
		padded.reserve(origSize);

		// 10-byte gzip header
		padded.insert(padded.end(), compressed.data(), compressed.data() + 10);

		if (padNeeded > 0)
		{
			padded[3] |= 0x10; // set FCOMMENT flag (FLG bit 4)

			if (padNeeded > 1)
				padded.insert(padded.end(), padNeeded - 1, ' ');

			padded.push_back('\0'); // null terminator
		}

		// DEFLATE data + CRC32 + ISIZE (everything after the header)
		padded.insert(padded.end(), compressed.data() + 10,
		              compressed.data() + compressed.size());

		// Write in-place
		DWORD oldProt = 0;
		const auto dst = const_cast<char*>(fontParams.gzipPtr);
		if (!VirtualProtect(dst, origSize, PAGE_READWRITE, &oldProt))
		{
			Logger::Log("[Font] ERROR: VirtualProtect failed for font region (err %lu).",
			            GetLastError());
			return;
		}

		std::memcpy(dst, padded.data(), padded.size());
		VirtualProtect(dst, origSize, oldProt, &oldProt);

		Logger::Log("[Font] Patched font gzip in memory (%zu bytes, padded to %zu).",
		            compressed.size(), padded.size());
	}
	else
	{
		// Recompressed font exceeds original space - allocate a new buffer
		// and redirect any pointer-table entries (translations approach).
		Logger::Log("[Font] Recompressed font exceeds original space, using pointer redirection.");

		const size_t allocSize = (std::max)(compressed.size(), static_cast<size_t>(4096));
		void* newGzip = VirtualAlloc(nullptr, allocSize,
		                             MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!newGzip)
		{
			Logger::Log("[Font] ERROR: VirtualAlloc failed for font buffer (err %lu).",
			            GetLastError());
			return;
		}
		std::memcpy(newGzip, compressed.data(), compressed.size());

		DWORD tmpProt;
		VirtualProtect(newGzip, allocSize, PAGE_READONLY, &tmpProt);

		const auto origVA = reinterpret_cast<uintptr_t>(fontParams.gzipPtr);
		const auto newVA = reinterpret_cast<uintptr_t>(newGzip);

		const uintptr_t rdataStart = fontParams.baseAddr + RDATA_RVA;
		const uintptr_t dataStart = fontParams.baseAddr + DATA_RVA;

		DWORD oldProtRdata = 0, oldProtData = 0;
		VirtualProtect(reinterpret_cast<void*>(rdataStart),
		               RDATA_SIZE, PAGE_READWRITE, &oldProtRdata);
		VirtualProtect(reinterpret_cast<void*>(dataStart),
		               DATA_SIZE, PAGE_READWRITE, &oldProtData);

		int ptrCount = 0;
		ptrCount += PatchPointersInRegion(rdataStart, RDATA_SIZE, origVA, newVA);
		ptrCount += PatchPointersInRegion(dataStart, DATA_SIZE, origVA, newVA);

		VirtualProtect(reinterpret_cast<void*>(rdataStart),
		               RDATA_SIZE, oldProtRdata, &oldProtRdata);
		VirtualProtect(reinterpret_cast<void*>(dataStart),
		               DATA_SIZE, oldProtData, &oldProtData);

		Logger::Log("[Font] Patched font gzip via allocation (%zu bytes, %d ptr(s) redirected).",
		            compressed.size(), ptrCount);

		if (ptrCount == 0)
			Logger::Log("[Font] WARN: No pointers found to redirect. "
				"Font may not be applied for sizes exceeding original.");
	}
}
