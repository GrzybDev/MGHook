#pragma once

#include <cstddef>
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
}
