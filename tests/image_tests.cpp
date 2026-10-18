#include <algorithm>

#include <gtest/gtest.h>
#include <windows.h>

#include "game/image.hpp"

// An absolute address kept in data, which the loader relocates like the game's string pointers.
const char* volatile gRelocatedPointer = "relocated";

namespace mghook::test
{
	TEST(ModuleImage, ListsTheSlotsTheLoaderRelocated)
	{
		const ModuleImage image(GetModuleHandleW(nullptr));
		const std::vector<std::uintptr_t*> slots = image.relocatedPointers();

		EXPECT_NE(std::ranges::find(slots, reinterpret_cast<std::uintptr_t*>(const_cast<const char**>(&gRelocatedPointer))),
		          slots.end());
	}

	TEST(ModuleImage, FindsTheImportSlotACallGoesThrough)
	{
		const ModuleImage image(GetModuleHandleW(nullptr));

		void** slot = image.importSlot("kernel32.dll", "GetModuleHandleW");

		ASSERT_NE(slot, nullptr);
		EXPECT_EQ(*slot, reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetModuleHandleW")));
		EXPECT_EQ(image.importSlot("kernel32.dll", "NoSuchFunction"), nullptr);
	}

	TEST(ModuleImage, FindsSectionsByName)
	{
		const ModuleImage image(GetModuleHandleW(nullptr));
		const std::span<std::byte> data = image.section(".data");
		const auto start = reinterpret_cast<std::uintptr_t>(data.data());
		const auto variable = reinterpret_cast<std::uintptr_t>(&gRelocatedPointer);

		EXPECT_TRUE(variable >= start && variable < start + data.size());
		EXPECT_TRUE(image.section(".none").empty());
	}

	TEST(WriteMemory, WritesReadOnlyMemoryAndKeepsItReadOnly)
	{
		auto* page = static_cast<std::uint32_t*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		ASSERT_NE(page, nullptr);
		DWORD protection;
		VirtualProtect(page, 4096, PAGE_READONLY, &protection);

		writeValue(page + 1, std::uint32_t{0xC0FFEE});

		MEMORY_BASIC_INFORMATION information;
		VirtualQuery(page, &information, sizeof(information));
		EXPECT_EQ(page[1], 0xC0FFEEu);
		EXPECT_EQ(information.Protect, static_cast<DWORD>(PAGE_READONLY));
		VirtualFree(page, 0, MEM_RELEASE);
	}
}
