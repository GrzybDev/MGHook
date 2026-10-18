#include "pch.hpp"
#include "image.hpp"

namespace mghook
{
	namespace
	{
		bool equalsIgnoringCase(const std::string_view left, const std::string_view right)
		{
			return std::ranges::equal(left, right, [](const char a, const char b)
			{
				return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
			});
		}
	}

	ModuleImage::ModuleImage(const HMODULE module) : base_(reinterpret_cast<std::byte*>(module))
	{
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base_);
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
			throw std::runtime_error("The module has no DOS header");

		headers_ = pointer<const IMAGE_NT_HEADERS64>(static_cast<std::uint32_t>(dos->e_lfanew));
		if (headers_->Signature != IMAGE_NT_SIGNATURE || headers_->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			throw std::runtime_error("The module is no 64-bit PE image");
	}

	std::uint32_t ModuleImage::timestamp() const
	{
		return headers_->FileHeader.TimeDateStamp;
	}

	std::span<std::byte> ModuleImage::section(const std::string_view name) const
	{
		const std::span sections(IMAGE_FIRST_SECTION(headers_), headers_->FileHeader.NumberOfSections);
		for (const IMAGE_SECTION_HEADER& section : sections)
		{
			const auto* raw = reinterpret_cast<const char*>(section.Name);
			if (std::string_view(raw, strnlen(raw, IMAGE_SIZEOF_SHORT_NAME)) == name)
				return at(section.VirtualAddress, section.Misc.VirtualSize);
		}

		return {};
	}

	std::span<std::byte> ModuleImage::at(const std::uint32_t rva, const std::size_t size) const
	{
		if (const std::size_t imageSize = headers_->OptionalHeader.SizeOfImage; rva > imageSize || size > imageSize - rva)
			throw std::out_of_range(std::format("RVA {:#x} + {:#x} is outside the image", rva, size));

		return {base_ + rva, size};
	}

	std::vector<std::uintptr_t*> ModuleImage::relocatedPointers() const
	{
		const IMAGE_DATA_DIRECTORY& directory = headers_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		const std::span<std::byte> table = at(directory.VirtualAddress, directory.Size);

		std::vector<std::uintptr_t*> slots;
		for (std::size_t offset = 0; offset + sizeof(IMAGE_BASE_RELOCATION) <= table.size();)
		{
			const auto* block = reinterpret_cast<const IMAGE_BASE_RELOCATION*>(table.data() + offset);
			if (block->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) || block->SizeOfBlock > table.size() - offset)
				break;

			const std::span<std::byte> entryBytes =
				table.subspan(offset + sizeof(IMAGE_BASE_RELOCATION), block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION));
			const std::span entries(reinterpret_cast<const std::uint16_t*>(entryBytes.data()),
			                        entryBytes.size() / sizeof(std::uint16_t));
			for (const std::uint16_t entry : entries)
			{
				if (entry >> 12 == IMAGE_REL_BASED_DIR64)
					slots.push_back(pointer<std::uintptr_t>(block->VirtualAddress + (entry & 0xFFF)));
			}

			offset += block->SizeOfBlock;
		}

		return slots;
	}

	void** ModuleImage::importSlot(const std::string_view library, const std::string_view function) const
	{
		const IMAGE_DATA_DIRECTORY& directory = headers_->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		if (directory.VirtualAddress == 0)
			return nullptr;

		for (const auto* import = pointer<const IMAGE_IMPORT_DESCRIPTOR>(directory.VirtualAddress); import->Name; ++import)
		{
			// Without the name table the bound address table is all that is left, and it names nothing.
			if (!import->OriginalFirstThunk || !equalsIgnoringCase(pointer<const char>(import->Name), library))
				continue;

			const auto* names = pointer<const IMAGE_THUNK_DATA64>(import->OriginalFirstThunk);
			auto* slots = pointer<IMAGE_THUNK_DATA64>(import->FirstThunk);
			for (; names->u1.AddressOfData; ++names, ++slots)
			{
				if (IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal))
					continue;

				const auto* byName = pointer<const IMAGE_IMPORT_BY_NAME>(static_cast<std::uint32_t>(names->u1.AddressOfData));
				if (function == byName->Name)
					return reinterpret_cast<void**>(&slots->u1.Function);
			}
		}

		return nullptr;
	}

	void writeMemory(void* destination, const std::span<const std::byte> bytes)
	{
		DWORD protection;
		if (!VirtualProtect(destination, bytes.size(), PAGE_READWRITE, &protection))
			throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "VirtualProtect");

		std::memcpy(destination, bytes.data(), bytes.size());
		VirtualProtect(destination, bytes.size(), protection, &protection);
	}
}
