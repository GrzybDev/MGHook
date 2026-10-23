#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace mghook
{
	class ModuleImage;

	/// The engine's uncompress_gzip, which Engine.dll exports and which returns zlib's codes.
	using UncompressGzip = int (*)(unsigned char* destination, unsigned int* destinationSize, unsigned char* source,
	                               unsigned int sourceSize);

	/// The decorated name the executable imports uncompress_gzip by.
	inline constexpr std::string_view kUncompressGzipImport = "?uncompress_gzip@@YAHPEAEPEAIQEAEI@Z";

	/// The engine's UTF-8 font as the executable carries it: a gzip member named MGS_Font_nht.raw.
	struct BuiltInFont
	{
		std::span<std::byte> member;
		std::string content;
	};

	/// Finds the built-in font.
	/// @param section The bytes to search, the executable's .data section.
	/// @return The font, or nothing when no member of that name is there.
	/// @throws std::runtime_error when the member is damaged.
	[[nodiscard]] std::optional<BuiltInFont> findBuiltInFont(std::span<std::byte> section);

	/// Hands the engine a replacement font when it unpacks its built-in one.
	class FontSubstitute
	{
	public:
		/// Prepares the replacement for one built-in member, remembering the member's size field so that the fallback
		/// can restore it.
		/// @param member The built-in font's gzip member.
		/// @param font The replacement font.
		/// @param original The engine's function, which unpacks everything else.
		FontSubstitute(std::span<std::byte> member, std::string font, UncompressGzip original);

		/// Sets the built-in member's size field to the replacement's size, since the engine sizes its buffer by it.
		/// @throws std::system_error when the field cannot be written.
		void announceSize() const;

		/// Does what uncompress_gzip does, except that the built-in font comes out as the replacement when the engine
		/// made room for it.
		/// @param destination Where the content goes.
		/// @param destinationSize The room at the destination, then the size written there.
		/// @param source The gzip member to unpack.
		/// @param sourceSize The member's size.
		int uncompress(unsigned char* destination, unsigned int* destinationSize, unsigned char* source,
		               unsigned int sourceSize) const noexcept;

	private:
		void writeSizeField(std::uint32_t size) const;

		std::span<std::byte> member_;
		std::uint32_t builtInSize_;
		std::string font_;
		UncompressGzip original_;
	};

	/// Makes the game draw its UTF-8 text with the font in a file, or exports the built-in font to that file.
	/// @param image The game's executable.
	/// @param file The replacement font, a TNOF font as MGTools writes it.
	/// @param exportWhenMissing Whether to write the built-in font to the file when there is none.
	void replaceFont(const ModuleImage& image, const std::filesystem::path& file, bool exportWhenMissing);
}
