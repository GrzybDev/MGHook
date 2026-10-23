#include "pch.hpp"
#include "gzip.hpp"

namespace mghook
{
	GzipMember inflateGzipMember(const std::span<const std::byte> data)
	{
		z_stream stream{};
		stream.next_in = reinterpret_cast<const Bytef*>(data.data());
		stream.avail_in = static_cast<uInt>(std::min<std::size_t>(data.size(), UINT_MAX));

		// 16 + the largest window accepts a gzip header and nothing else.
		if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK)
			throw std::runtime_error("zlib could not start inflating");
		const std::unique_ptr<z_stream, decltype(&inflateEnd)> inflation(&stream, &inflateEnd);

		std::string content;
		int result;
		do
		{
			std::array<char, 1 << 16> chunk;
			stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
			stream.avail_out = static_cast<uInt>(chunk.size());
			result = inflate(&stream, Z_NO_FLUSH);
			content.append(chunk.data(), chunk.size() - stream.avail_out);
		}
		while (result == Z_OK);

		if (result != Z_STREAM_END)
			throw std::runtime_error(std::format("The gzip member is damaged or cut short (zlib error {})", result));

		return {.size = stream.total_in, .content = std::move(content)};
	}

	std::uint32_t gzipSizeField(const std::span<const std::byte> member)
	{
		std::uint32_t size;
		std::memcpy(&size, member.last(sizeof(size)).data(), sizeof(size));
		return size;
	}
}
