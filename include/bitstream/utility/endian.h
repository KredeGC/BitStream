#pragma once

#include <cstdint>

#if __cplusplus < 202002L
#ifndef BS_LITTLE_ENDIAN
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define BS_LITTLE_ENDIAN false
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define BS_LITTLE_ENDIAN true
#else
#error "I don't know what architecture this is!"
#endif
#endif // BS_LITTLE_ENDIAN
#else // __cplusplus < 202002L
#include <bit>
#endif // __cplusplus < 202002L


#ifdef _WIN32
#include <intrin.h>
#endif

namespace bitstream::utility
{
    inline constexpr bool little_endian()
    {
#ifdef BS_LITTLE_ENDIAN
#if BS_LITTLE_ENDIAN
        return true;
#else // BS_LITTLE_ENDIAN
        return false;
#endif // BS_LITTLE_ENDIAN
#else // defined(BS_LITTLE_ENDIAN)
        return std::endian::native == std::endian::little;
#endif // defined(BS_LITTLE_ENDIAN)
    }

	inline constexpr uint32_t endian_swap_32(uint32_t value)
	{
        if constexpr (little_endian())
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
        if constexpr (little_endian())
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