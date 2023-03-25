#pragma once

#include <cstdint>

namespace bitstream
{
	/**
	 * @brief A byte buffer aligned to 4 bytes.
	 * Can be used with bit_reader and bit_writer.
	 * @note Size must be a multiple of 4
	*/
	template<size_t Size>
	struct byte_buffer
	{
		static_assert(Size % 4 == 0, "Buffer size must be a multiple of 4");

		alignas(uint32_t) uint8_t Bytes[Size];
	};
}