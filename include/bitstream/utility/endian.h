#pragma once

#include <cstdint>


#ifdef _WIN32
#include <intrin.h>
#endif

namespace bitstream::utility
{
	const inline bool LITTLE_ENDIANNESS = []()
	{
		union
		{
			uint16_t i;
			char c[2];
		} bint = { 0x0102 };

		return bint.c[0] != 0x01;
	}();

	inline constexpr uint32_t endian_swap_32(uint32_t value)
	{
		if (LITTLE_ENDIANNESS)
		{
#if defined(_WIN32)
            return _byteswap_ulong(value);
#elif defined(__linux__)
            return __builtin_bswap32(value);
#else
			const uint32_t first = (value << 24)	& 0xFF000000;
			const uint32_t second = (value << 8)	& 0x00FF0000;
			const uint32_t third = (value >> 8)		& 0x0000FF00;
			const uint32_t fourth = (value >> 24)	& 0x000000FF;

			return first | second | third | fourth;
#endif // _WIN32 || __linux__
		}

		return value;
	}

	inline constexpr uint32_t endian_swap_16(uint32_t value)
	{
		if (LITTLE_ENDIANNESS)
		{
#if defined(_WIN32)
            return _byteswap_ushort(value);
#elif defined(__linux__)
            return __builtin_bswap16(value);
#else
			const uint32_t first = (value << 8) & 0xFF00;
			const uint32_t second = (value >> 8) & 0x00FF;

			return first | second;
#endif // _WIN32 || __linux__
		}

		return value;
	}
}