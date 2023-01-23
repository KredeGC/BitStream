#pragma once

#include <cstddef>
#include <cstdint>

namespace bitstream::utility
{
	constexpr inline size_t bits_to_represent(size_t n)
	{
		size_t r = 0;

		if (n >> 32) { r += 32; n >>= 32; }
		if (n >> 16) { r += 16; n >>= 16; }
		if (n >> 8) { r += 8; n >>= 8; }
		if (n >> 4) { r += 4; n >>= 4; }
		if (n >> 2) { r += 2; n >>= 2; }
		if (n - 1) ++r;

		return r;
	}

	constexpr inline size_t bits_in_range(size_t min, size_t max)
	{
		return bits_to_represent(max - min);
	}
}