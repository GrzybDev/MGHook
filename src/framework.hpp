#pragma once

static auto VERSION = "0.1.0";

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files
#include <windows.h>
#include <tchar.h>

// Standard Library Headers
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <string>

// Gzip Library Headers
#include <gzip/compress.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <zlib.h>

// Sections to scan for pointer-table entries
static constexpr DWORD RDATA_RVA = 0x124000;
static constexpr DWORD RDATA_SIZE = 0x50400;
static constexpr DWORD DATA_RVA = 0x175000;
static constexpr DWORD DATA_SIZE = 0x20D7438;

// String region descriptor
struct StringRegion
{
	DWORD rvaStart;
	DWORD rvaEnd;
	const char* name;
};

// All string regions to scan
static constexpr StringRegion STRING_REGIONS[] = {
	{0x14D240, 0x14D2AF, "Region1-Labels(multi-lang)"}, // file 0x14B840-0x14B8AF
	{0x14D2B0, 0x14DA97, "Region1-Locations(EN)"}, // file 0x14B8B0-0x14C097
	{0x14DA98, 0x14E21F, "Region1-Locations(FR)"}, // file 0x14C098-0x14C81F
	{0x14E220, 0x14E8FF, "Region1-Locations(DE)"}, // file 0x14C820-0x14CEFF
	{0x14E900, 0x14F10F, "Region1-Locations(IT)"}, // file 0x14CF00-0x14D70F
	{0x14F110, 0x14F937, "Region1-Locations(ES)"}, // file 0x14D710-0x14DF37
	{0x14F938, 0x14FF58, "Region1-Locations(JP)"}, // file 0x14DF38-0x14E558
	{0x1502A0, 0x15537C, "Region2-UI(multi-lang)"}, // file 0x14E8A0-0x15397C
	{0x155E40, 0x15708F, "Region3-SaveManagement(multi-lang)"}, // file 0x154440-0x15568F
};

static constexpr int NUM_STRING_REGIONS = std::size(STRING_REGIONS);

struct CharMapEntry
{
	std::string from; // UTF-8 bytes of source character
	std::string to; // UTF-8 bytes of replacement character
};

static std::vector<CharMapEntry> charMap;
static std::vector<char*> allocatedStrings;

struct TranslationEntry
{
	std::string original; // key as raw UTF-8 bytes
	std::string replacement; // value as raw UTF-8 bytes
};

struct AddressTuple
{
	uintptr_t rdataStart;
	uintptr_t dataStart;
	uintptr_t baseAdr;
};

struct TranslationResult
{
	int patchedCount;
	int skippedCount;
	int totalPtrsPatched;
};

static constexpr DWORD FONT_GZIP_RVA = 0x1797C0;
static constexpr size_t FONT_GZIP_MAX_SIZE = 0xF1230; // conservative upper bound

struct FontParameters
{
	std::wstring engineDir;
	std::wstring fontsDir;
	std::wstring fontPath;

	const char* gzipPtr;
	size_t origGzipSize;
	uintptr_t baseAddr;
};
