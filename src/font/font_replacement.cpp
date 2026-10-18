#include "pch.hpp"
#include "font_replacement.hpp"

#include "app/log.hpp"
#include "font/gzip.hpp"
#include "game/image.hpp"
#include "util/files.hpp"

namespace mghook
{
	namespace
	{
		constexpr std::string_view kMemberName{"MGS_Font_nht.raw\0", 17};
		constexpr std::string_view kTnofMagic = "TNOF";
		// The gzip header up to the file name: magic, deflate, flags with only FNAME set.
		constexpr std::array kHeaderStart{std::byte{0x1F}, std::byte{0x8B}, std::byte{0x08}, std::byte{0x08}};
		constexpr std::size_t kNameOffset = 10;

		const FontSubstitute* gSubstitute = nullptr;

		int uncompressGzip(unsigned char* destination, unsigned int* destinationSize, unsigned char* source,
		                   const unsigned int sourceSize) noexcept
		{
			return gSubstitute->uncompress(destination, destinationSize, source, sourceSize);
		}
	}

	std::optional<BuiltInFont> findBuiltInFont(const std::span<std::byte> section)
	{
		const auto name = std::as_bytes(std::span(kMemberName));
		for (auto at = section.begin(); (at = std::search(at, section.end(), name.begin(), name.end())) != section.end(); ++at)
		{
			const auto offset = static_cast<std::size_t>(at - section.begin());
			if (offset < kNameOffset || !std::ranges::equal(section.subspan(offset - kNameOffset, kHeaderStart.size()), kHeaderStart))
				continue;

			const std::span member = section.subspan(offset - kNameOffset);
			GzipMember inflated = inflateGzipMember(member);
			return BuiltInFont{.member = member.first(inflated.size), .content = std::move(inflated.content)};
		}

		return std::nullopt;
	}

	FontSubstitute::FontSubstitute(const std::span<std::byte> member, std::string font, const UncompressGzip original)
		: member_(member), font_(std::move(font)), original_(original)
	{
		std::memcpy(&builtInSize_, member_.last(sizeof(builtInSize_)).data(), sizeof(builtInSize_));
	}

	void FontSubstitute::announceSize() const
	{
		writeSizeField(static_cast<std::uint32_t>(font_.size()));
	}

	int FontSubstitute::uncompress(unsigned char* destination, unsigned int* destinationSize, unsigned char* source,
	                               const unsigned int sourceSize) const noexcept
	{
		if (source != reinterpret_cast<unsigned char*>(member_.data()))
			return original_(destination, destinationSize, source, sourceSize);

		if (*destinationSize < font_.size())
		{
			log::error("The game made room for {} bytes of font instead of {}, so it keeps its own font",
			           *destinationSize, font_.size());
			try
			{
				writeSizeField(builtInSize_);
			}
			catch (const std::system_error& error)
			{
				log::error("The built-in font's size field could not be restored: {}", error.what());
			}

			return original_(destination, destinationSize, source, sourceSize);
		}

		std::memcpy(destination, font_.data(), font_.size());
		*destinationSize = static_cast<unsigned int>(font_.size());
		log::info("Handed the game the replacement font");
		return Z_OK;
	}

	void FontSubstitute::writeSizeField(const std::uint32_t size) const
	{
		writeValue(member_.last(sizeof(size)).data(), size);
	}

	void replaceFont(const ModuleImage& image, const std::filesystem::path& file, const bool exportWhenMissing)
	{
		std::optional<BuiltInFont> builtIn = findBuiltInFont(image.section(".data"));
		if (!builtIn)
		{
			log::error("The game's built-in font was not found, so the font stays as it is");
			return;
		}

		if (!std::filesystem::exists(file))
		{
			if (exportWhenMissing)
			{
				writeFile(file, builtIn->content);
				log::info("Exported the built-in font to {}", log::pathText(file));
			}
			else
				log::info("No font at {}, so the game keeps its own", log::pathText(file));

			return;
		}

		std::optional<std::string> font = readFile(file);
		if (!font || !font->starts_with(kTnofMagic) || font->size() > UINT_MAX)
		{
			log::error("{} cannot be read or is no TNOF font, so the game keeps its own", log::pathText(file));
			return;
		}

		void** slot = image.importSlot("Engine.dll", kUncompressGzipImport);
		if (!slot)
		{
			log::error("The game does not import uncompress_gzip from Engine.dll, so it keeps its own font");
			return;
		}

		// The engine unpacks the font after DllMain returns, so the substitute is never freed.
		gSubstitute = new FontSubstitute(builtIn->member, std::move(*font), reinterpret_cast<UncompressGzip>(*slot));
		writeValue(slot, reinterpret_cast<void*>(&uncompressGzip));
		// Last, so that if it fails the engine makes room for its own font, which the substitute then falls back to.
		gSubstitute->announceSize();
		log::info("Replacing the built-in font with {}", log::pathText(file));
	}
}
