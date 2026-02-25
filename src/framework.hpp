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
	{0x14A6E8, 0x14A817, "Region0-Difficulty"}, // file 0x148CE8-0x148E17
	{0x14D240, 0x14D2AF, "Region1-SaveLabels"}, // file 0x14B840-0x14B8AF
	{0x1502A0, 0x15537C, "Region2-UI"}, // file 0x14E8A0-0x15397C
	{0x155E40, 0x15708F, "Region3-SaveManagement"}, // file 0x154440-0x15568F
};

static constexpr int NUM_STRING_REGIONS = std::size(STRING_REGIONS);

static constexpr DWORD FONT_GZIP_RVA = 0x1797C0;
static constexpr size_t FONT_GZIP_MAX_SIZE = 0xF1230; // conservative upper bound
