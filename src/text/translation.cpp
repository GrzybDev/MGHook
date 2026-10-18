#include "pch.hpp"
#include "translation.hpp"

#include "game/image.hpp"

namespace mghook
{
	namespace
	{
		// The room passed on is the string with its terminator and padding; a string the region cuts off is left out.
		template <typename Visit>
		void forEachString(const std::span<std::byte> bytes, Visit visit)
		{
			const auto isZero = [](const std::byte value) { return value == std::byte{0}; };
			auto start = std::find_if_not(bytes.begin(), bytes.end(), isZero);
			while (start != bytes.end())
			{
				const auto terminator = std::find_if(start, bytes.end(), isZero);
				if (terminator == bytes.end())
					return;

				const auto next = std::find_if_not(terminator, bytes.end(), isZero);
				visit(std::span(start, next), std::string_view(reinterpret_cast<const char*>(&*start),
				                                               static_cast<std::size_t>(terminator - start)));
				start = next;
			}
		}
	}

	TextPlan planTranslation(const std::span<const MappedRegion> regions, const std::span<const TextEntry> translations)
	{
		std::unordered_map<std::string_view, const TextEntry*> byOriginal;
		for (const TextEntry& entry : translations)
			byOriginal.emplace(entry.key, &entry);

		TextPlan plan;
		std::unordered_set<const TextEntry*> matched;
		for (const MappedRegion& region : regions)
		{
			forEachString(region.bytes, [&](const std::span<std::byte> room, const std::string_view text)
			{
				if (const auto entry = byOriginal.find(text); entry != byOriginal.end())
				{
					plan.patches.push_back({.region = region.name, .room = room, .original = std::string(text),
					                        .translation = region.charMap.apply(entry->second->value)});
					matched.insert(entry->second);
				}
			});
		}

		for (const TextEntry& entry : translations)
		{
			if (!matched.contains(&entry))
				plan.unmatched.push_back(entry);
		}

		return plan;
	}

	TextResult applyTranslation(const std::span<const TextPatch> patches, const std::span<std::uintptr_t* const> pointers)
	{
		TextResult result;
		std::vector<const TextPatch*> moving;
		std::size_t movedSize = 0;
		for (const TextPatch& patch : patches)
		{
			if (patch.fitsInPlace())
			{
				std::vector<std::byte> room(patch.room.size());
				std::memcpy(room.data(), patch.translation.data(), patch.translation.size());
				writeMemory(patch.room.data(), room);
				++result.inPlace;
				continue;
			}

			moving.push_back(&patch);
			movedSize += patch.translation.size() + 1;
		}

		if (moving.empty())
			return result;

		std::unordered_map<std::uintptr_t, std::vector<std::uintptr_t*>> pointersTo;
		for (const TextPatch* patch : moving)
			pointersTo.try_emplace(reinterpret_cast<std::uintptr_t>(patch->room.data()));
		for (std::uintptr_t* pointer : pointers)
		{
			if (const auto target = pointersTo.find(*pointer); target != pointersTo.end())
				target->second.push_back(pointer);
		}

		// The game reads these strings for as long as it runs, so they are never freed.
		char* storage = new char[movedSize];
		for (const TextPatch* patch : moving)
		{
			const std::vector<std::uintptr_t*>& references = pointersTo[reinterpret_cast<std::uintptr_t>(patch->room.data())];
			if (references.empty())
			{
				result.stranded.push_back(patch->original);
				continue;
			}

			std::memcpy(storage, patch->translation.c_str(), patch->translation.size() + 1);
			for (std::uintptr_t* reference : references)
				writeValue(reference, reinterpret_cast<std::uintptr_t>(storage));

			storage += patch->translation.size() + 1;
			result.redirectedPointers += references.size();
			++result.moved;
		}

		return result;
	}
}
