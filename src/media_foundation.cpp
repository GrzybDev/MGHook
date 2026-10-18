#include "pch.hpp"

#include <mfidl.h>
#include <mfreadwrite.h>

// Every export of the system's MFReadWrite.dll, passed on to it: besides the game, the Unity launcher in the same
// folder loads this library by name for its videos and asks for the others.

namespace
{
	HMODULE systemLibrary()
	{
		// Loaded on the first call rather than in DllMain, where loading a library can deadlock.
		static const HMODULE library = []() -> HMODULE
		{
			std::wstring folder(MAX_PATH, L'\0');
			const UINT length = GetSystemDirectoryW(folder.data(), static_cast<UINT>(folder.size()));
			// Without the system folder the name alone would find this library again.
			if (length == 0 || length >= folder.size())
				return nullptr;

			folder.resize(length);
			return LoadLibraryW((std::filesystem::path(folder) / L"MFReadWrite.dll").c_str());
		}();
		return library;
	}

	template <typename Function>
	Function systemFunction(const char* name)
	{
		const HMODULE library = systemLibrary();
		return library ? reinterpret_cast<Function>(GetProcAddress(library, name)) : nullptr;
	}

	template <typename Function, typename... Args>
	HRESULT callSystem(const Function function, Args... args)
	{
		return function ? function(args...) : HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
	}
}

HRESULT WINAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource* source, IMFAttributes* attributes,
                                                   IMFSourceReader** reader)
{
	static const auto function = systemFunction<decltype(&MFCreateSourceReaderFromMediaSource)>(__func__);
	return callSystem(function, source, attributes, reader);
}

HRESULT WINAPI MFCreateSourceReaderFromURL(LPCWSTR url, IMFAttributes* attributes, IMFSourceReader** reader)
{
	static const auto function = systemFunction<decltype(&MFCreateSourceReaderFromURL)>(__func__);
	return callSystem(function, url, attributes, reader);
}

HRESULT WINAPI MFCreateSourceReaderFromByteStream(IMFByteStream* stream, IMFAttributes* attributes,
                                                  IMFSourceReader** reader)
{
	static const auto function = systemFunction<decltype(&MFCreateSourceReaderFromByteStream)>(__func__);
	return callSystem(function, stream, attributes, reader);
}

HRESULT WINAPI MFCreateSinkWriterFromURL(LPCWSTR url, IMFByteStream* stream, IMFAttributes* attributes,
                                         IMFSinkWriter** writer)
{
	static const auto function = systemFunction<decltype(&MFCreateSinkWriterFromURL)>(__func__);
	return callSystem(function, url, stream, attributes, writer);
}

HRESULT WINAPI MFCreateSinkWriterFromMediaSink(IMFMediaSink* sink, IMFAttributes* attributes, IMFSinkWriter** writer)
{
	static const auto function = systemFunction<decltype(&MFCreateSinkWriterFromMediaSink)>(__func__);
	return callSystem(function, sink, attributes, writer);
}

HRESULT WINAPI DllGetClassObject(REFCLSID classId, REFIID interfaceId, LPVOID* object)
{
	static const auto function = systemFunction<decltype(&DllGetClassObject)>(__func__);
	return callSystem(function, classId, interfaceId, object);
}

HRESULT WINAPI DllCanUnloadNow()
{
	static const auto function = systemFunction<decltype(&DllCanUnloadNow)>(__func__);
	return callSystem(function);
}
