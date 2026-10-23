#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace mghook
{
	/// A gzip member inflated from the start of a buffer.
	struct GzipMember
	{
		/// Its compressed size, header and trailer included.
		std::size_t size;
		std::string content;
	};

	/// Inflates the gzip member a buffer starts with; the buffer may go on with unrelated bytes.
	/// @param data The buffer, starting with the member's header.
	/// @throws std::runtime_error when the buffer does not start with a whole gzip member.
	GzipMember inflateGzipMember(std::span<const std::byte> data);

	/// Reads the field a gzip member ends with, the size of its content modulo 2^32.
	/// @param member The whole member.
	[[nodiscard]] std::uint32_t gzipSizeField(std::span<const std::byte> member);
}
