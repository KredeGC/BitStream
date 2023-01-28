#pragma once
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace bitstream
{
	template<typename T, T, T>
	struct const_int; // TODO: Rename to bound_int

#pragma region integral types
	template<typename T>
	struct serialize_traits<T, typename std::enable_if_t<std::is_integral_v<T>>>
	{
		static bool serialize(bit_writer& writer, const T& value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)()) noexcept
		{
			BS_ASSERT(min < max);
            
            BS_ASSERT(value >= min && value < max);

			int num_bits = static_cast<int>(utility::bits_in_range(min, max));

			BS_ASSERT(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4)
			{
				// If the given range is bigger than a word (32 bits)
				int max_words = (num_bits - 1) / 32 + 1;
				for (int i = 0; i < max_words; i++)
				{
					int shift = i * 32;
					int bit_count = (std::min)(num_bits - shift, 32);
					uint32_t unsigned_value = static_cast<uint32_t>((value - min) >> shift);
					BS_ASSERT(writer.serialize_bits(unsigned_value, bit_count));
				}
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value = static_cast<uint32_t>(value - min);
				BS_ASSERT(writer.serialize_bits(unsigned_value, num_bits));
			}

			return true;
		}

		static bool serialize(bit_reader& reader, T& value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)()) noexcept
		{
			BS_ASSERT(min < max);

			int num_bits = static_cast<int>(utility::bits_in_range(min, max));

			BS_ASSERT(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4)
			{
				value = 0;

				// If the given range is bigger than a word (32 bits)
				int max_words = (num_bits - 1) / 32 + 1;
				for (int i = 0; i < max_words; i++)
				{
					uint32_t unsigned_value;
					int shift = i * 32;
					int bit_count = (std::min)(num_bits - shift, 32);
					BS_ASSERT(reader.serialize_bits(unsigned_value, bit_count));

					value |= static_cast<T>(unsigned_value) << shift;
				}

				value += min;
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value;
				BS_ASSERT(reader.serialize_bits(unsigned_value, num_bits));

				value = static_cast<T>(unsigned_value) + min;
			}
            
            BS_ASSERT(value >= min && value < max);

			return true;
		}
	};
#pragma endregion

#pragma region const integral types
	template<typename T, T Min, T Max>
	struct serialize_traits<const_int<T, Min, Max>, typename std::enable_if_t<std::is_integral_v<T>>>
	{
		static bool serialize(bit_writer& writer, const T& value) noexcept
		{
			static_assert(Min < Max);
            
            BS_ASSERT(value >= Min && value < Max);

			constexpr int num_bits = static_cast<int>(utility::bits_in_range(Min, Max));

			static_assert(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4 && num_bits > 32)
			{
				// If the given range is bigger than a word (32 bits)
				int max_words = (num_bits - 1) / 32 + 1;
				for (int i = 0; i < max_words; i++)
				{
					int shift = i * 32;
					int bit_count = (std::min)(num_bits - shift, 32);
					uint32_t unsigned_value = static_cast<uint32_t>((value - Min) >> shift);
					BS_ASSERT(writer.serialize_bits(unsigned_value, bit_count));
				}
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value = static_cast<uint32_t>(value - Min);
				BS_ASSERT(writer.serialize_bits(unsigned_value, num_bits));
			}

			return true;
		}

		static bool serialize(bit_reader& reader, T& value) noexcept
		{
			static_assert(Min < Max);

			constexpr int num_bits = static_cast<int>(utility::bits_in_range(Min, Max));

			static_assert(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4 && num_bits > 32)
			{
				value = 0;

				// If the given range is bigger than a word (32 bits)
				int max_words = (num_bits - 1) / 32 + 1;
				for (int i = 0; i < max_words; i++)
				{
					uint32_t unsigned_value;
					int shift = i * 32;
					int bit_count = (std::min)(num_bits - shift, 32);
					BS_ASSERT(reader.serialize_bits(unsigned_value, bit_count));

					value |= static_cast<T>(unsigned_value) << shift;
				}

				value += Min;
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value;
				BS_ASSERT(reader.serialize_bits(unsigned_value, num_bits));

				value = static_cast<T>(unsigned_value) + Min;
			}
            
            BS_ASSERT(value >= Min && value < Max);

			return true;
		}
	};
#pragma endregion
}