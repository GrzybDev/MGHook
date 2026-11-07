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

	// Export only keys the template: each forwarded function gets its own instantiation, so its own cached address.
	template <auto Export, typename... Args>
	HRESULT callSystem(const char* name, Args... args)
	{
		static const auto function = [name]() -> decltype(Export)
		{
			const HMODULE library = systemLibrary();
			return library ? reinterpret_cast<decltype(Export)>(GetProcAddress(library, name)) : nullptr;
		}();
		return function ? function(args...) : HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
	}
}

HRESULT WINAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource* source, IMFAttributes* attributes,
                                                   IMFSourceReader** reader)
{
	return callSystem<&MFCreateSourceReaderFromMediaSource>(__func__, source, attributes, reader);
}

HRESULT WINAPI MFCreateSourceReaderFromURL(LPCWSTR url, IMFAttributes* attributes, IMFSourceReader** reader)
{
	return callSystem<&MFCreateSourceReaderFromURL>(__func__, url, attributes, reader);
}

HRESULT WINAPI MFCreateSourceReaderFromByteStream(IMFByteStream* stream, IMFAttributes* attributes,
                                                  IMFSourceReader** reader)
{
	return callSystem<&MFCreateSourceReaderFromByteStream>(__func__, stream, attributes, reader);
}

HRESULT WINAPI MFCreateSinkWriterFromURL(LPCWSTR url, IMFByteStream* stream, IMFAttributes* attributes,
                                         IMFSinkWriter** writer)
{
	return callSystem<&MFCreateSinkWriterFromURL>(__func__, url, stream, attributes, writer);
}

HRESULT WINAPI MFCreateSinkWriterFromMediaSink(IMFMediaSink* sink, IMFAttributes* attributes, IMFSinkWriter** writer)
{
	return callSystem<&MFCreateSinkWriterFromMediaSink>(__func__, sink, attributes, writer);
}

HRESULT WINAPI DllGetClassObject(REFCLSID classId, REFIID interfaceId, LPVOID* object)
{
	return callSystem<&DllGetClassObject>(__func__, classId, interfaceId, object);
}

HRESULT WINAPI DllCanUnloadNow()
{
	return callSystem<&DllCanUnloadNow>(__func__);
}
