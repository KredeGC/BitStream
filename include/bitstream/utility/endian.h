#pragma once

#include <cstdint>

namespace bitstream::utility
{
	const inline bool LITTLE_ENDIAN = []()
	{
		union {
			uint16_t i;
			char c[2];
		} bint = { 0x0102 };

		return bint.c[0] != 1;
	}();

	inline uint32_t endian_swap_32(uint32_t value)
	{
		if (LITTLE_ENDIAN)
		{
			const uint32_t first = (value << 24)	& 0xFF000000;
			const uint32_t second = (value << 8)	& 0x00FF0000;
			const uint32_t third = (value >> 8)		& 0x0000FF00;
			const uint32_t fourth = (value >> 24)	& 0x000000FF;

			return first | second | third | fourth;
		}

		return value;
	}

	inline uint32_t endian_swap_24(uint32_t value)
	{
		if (LITTLE_ENDIAN)
		{
			const uint32_t first = (value << 16)	& 0xFF0000;
			const uint32_t second = value			& 0x00FF00;
			const uint32_t third = (value >> 16)	& 0x0000FF;

			return first | second | third;
		}

		return value;
	}

	inline uint32_t endian_swap_16(uint32_t value)
	{
		if (LITTLE_ENDIAN)
		{
			const uint32_t first = (value << 8) & 0xFF00;
			const uint32_t second = (value >> 8) & 0x00FF;

			return first | second;
		}

		return value;
	}
}