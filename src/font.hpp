#pragma once

struct FontParameters
{
	std::wstring engineDir;
	std::wstring fontsDir;
	std::wstring fontPath;

	const char* gzipPtr;
	size_t origGzipSize;
	uintptr_t baseAddr;
};


void ExtractFont(const FontParameters& fontParams);
void ReplaceFont(const FontParameters& fontParams);
