#pragma once

#include <cstddef>
#include <cstdint>

namespace bitstream::utility
{
	constexpr inline uint32_t bits_to_represent(uintmax_t n)
	{
		uint32_t r = 0;

		if (n >> 32) { r += 32U; n >>= 32U; }
		if (n >> 16) { r += 16U; n >>= 16U; }
		if (n >> 8) { r += 8U; n >>= 8U; }
		if (n >> 4) { r += 4U; n >>= 4U; }
		if (n >> 2) { r += 2U; n >>= 2U; }
		if (n >> 1) { r += 1U; n >>= 1U; }

		return r + static_cast<uint32_t>(n);
	}

	constexpr inline uint32_t bits_in_range(intmax_t min, intmax_t max)
	{
		return bits_to_represent(static_cast<uintmax_t>(max) - static_cast<uintmax_t>(min));
	}
}