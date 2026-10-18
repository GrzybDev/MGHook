#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include <windows.h>

namespace mghook
{
	/// A 64-bit PE image the loader has mapped, read through its headers.
	class ModuleImage
	{
	public:
		/// Reads the headers of a mapped image.
		/// @param module The image's base address.
		/// @throws std::runtime_error when the module is no 64-bit PE image.
		explicit ModuleImage(HMODULE module);

		/// Returns the link time the image's header records, which tells builds apart.
		[[nodiscard]] std::uint32_t timestamp() const;

		/// Returns the bytes a section occupies in memory.
		/// @param name The section's name, such as ".data".
		/// @return The section, or nothing when the image has no such section.
		[[nodiscard]] std::span<std::byte> section(std::string_view name) const;

		/// Returns the bytes at a relative virtual address.
		/// @param rva The address relative to the image base.
		/// @param size How many bytes.
		/// @throws std::out_of_range when the bytes are not all inside the image.
		[[nodiscard]] std::span<std::byte> at(std::uint32_t rva, std::size_t size) const;

		/// Lists the 64-bit slots the loader relocated, the absolute addresses the image keeps outside its code.
		[[nodiscard]] std::vector<std::uintptr_t*> relocatedPointers() const;

		/// Finds the import address table entry the image calls a function through.
		/// @param library The imported library's name, compared without case.
		/// @param function The function's exported name.
		/// @return The entry, or null when the image does not import the function by that name.
		[[nodiscard]] void** importSlot(std::string_view library, std::string_view function) const;

	private:
		template <typename T>
		T* pointer(const std::uint32_t rva) const
		{
			return reinterpret_cast<T*>(base_ + rva);
		}

		std::byte* base_;
		const IMAGE_NT_HEADERS64* headers_;
	};

	/// Overwrites memory whatever its protection, restoring the protection afterwards.
	/// @param destination The first byte to overwrite.
	/// @param bytes What to write there.
	/// @throws std::system_error when the protection cannot be changed.
	void writeMemory(void* destination, std::span<const std::byte> bytes);

	/// Overwrites a value in memory whatever its protection.
	/// @param destination Where the value is.
	/// @param value The new value.
	template <typename T>
	void writeValue(void* destination, const T& value)
	{
		writeMemory(destination, std::as_bytes(std::span(&value, 1)));
	}
}
