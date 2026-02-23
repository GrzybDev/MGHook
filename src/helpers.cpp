#include "pch.hpp"
#include "helpers.hpp"

void DebugConsole()
{
	// If ".debug" file exists in the current directory, enable console output for debugging purposes.
	std::ifstream debugFile(".debug");

	if (debugFile.good())
	{
		// Create a console for Debug output
		AllocConsole();

		// Redirect standard error, output to console
		// std::cout, std::clog, std::cerr, std::cin
		FILE* fDummy;

		(void)freopen_s(&fDummy, "CONOUT$", "w", stdout);
		(void)freopen_s(&fDummy, "CONOUT$", "w", stderr);
		(void)freopen_s(&fDummy, "CONIN$", "r", stdin);

		std::cout.clear();
		std::clog.clear();
		std::cerr.clear();
		std::cin.clear();

		// Redirect wide standard error, output to console
		// std::wcout, std::wclog, std::wcerr, std::wcin
		const HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE,
		                                  FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		                                  FILE_ATTRIBUTE_NORMAL, nullptr);
		const HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		                                 nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
		SetStdHandle(STD_ERROR_HANDLE, hConOut);
		SetStdHandle(STD_INPUT_HANDLE, hConIn);

		std::wcout.clear();
		std::wclog.clear();
		std::wcerr.clear();
		std::wcin.clear();
	}
}
