#pragma once

#include <cstdint>

#if __cplusplus < 202002L
#ifndef BS_LITTLE_ENDIAN
    // Detect with GCC 4.6's macro.
#   if defined(__BYTE_ORDER__)
#       if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#           define BS_LITTLE_ENDIAN true
#       elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#           define BS_LITTLE_ENDIAN false
#       else
#           error "Unknown machine byteorder endianness detected. Need to manually define BS_LITTLE_ENDIAN."
#       endif
    // Detect with GLIBC's endian.h.
#   elif defined(__GLIBC__)
#       include <endian.h>
#       if (__BYTE_ORDER == __LITTLE_ENDIAN)
#           define BS_LITTLE_ENDIAN true
#       elif (__BYTE_ORDER == __BIG_ENDIAN)
#           define BS_LITTLE_ENDIAN false
#       else
#           error "Unknown machine byteorder endianness detected. Need to manually define BS_LITTLE_ENDIAN."
#       endif
    // Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro.
#   elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#       define BS_LITTLE_ENDIAN true
#   elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#       define BS_LITTLE_ENDIAN false
    // Detect with architecture macros.
#   elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#       define BS_LITTLE_ENDIAN false
#   elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#       define BS_LITTLE_ENDIAN true
#   elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#       define BS_LITTLE_ENDIAN true
#   else
#       error "Unknown machine byteorder endianness detected. Need to manually define BS_LITTLE_ENDIAN."
#   endif
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

	inline uint32_t endian_swap_32(uint32_t value)
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

	inline uint32_t endian_swap_16(uint32_t value)
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