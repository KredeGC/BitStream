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
	struct const_int;

#pragma region integral types
	template<typename T>
	struct serialize_traits<T, typename std::enable_if_t<std::is_integral<T>::value>>
	{
		static bool serialize(bit_writer& writer, const T& value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)())
		{
			BS_ASSERT(min < max);

			if (value < min || value > max)
				return false;

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
					bool status = writer.serialize_bits(unsigned_value, bit_count);
					if (!status)
						return false;
				}
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value = static_cast<uint32_t>(value - min);
				bool status = writer.serialize_bits(unsigned_value, num_bits);
				if (!status)
					return false;
			}

			return true;
		}

		static bool serialize(bit_reader& reader, T& value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)())
		{
			BS_ASSERT(min < max);

			int num_bits = static_cast<int>(utility::bits_in_range(min, max));

			BS_ASSERT(num_bits <= sizeof(T) * 8);

			if (!reader.can_read_bits(num_bits))
				return false;

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
					bool status = reader.serialize_bits(unsigned_value, bit_count);
					if (!status)
						return false;

					value |= static_cast<T>(unsigned_value) << shift;
				}

				value += min;
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value;
				bool status = reader.serialize_bits(unsigned_value, num_bits);
				if (!status)
					return false;

				value = static_cast<T>(unsigned_value) + min;
			}

			if (value < min || value > max)
				return false;

			return true;
		}
	};
#pragma endregion

#pragma region const integral types
	template<typename T, T Min, T Max>
	struct serialize_traits<const_int<T, Min, Max>, typename std::enable_if_t<std::is_integral<T>::value>>
	{
		static bool serialize(bit_writer& writer, const T& value)
		{
			static_assert(Min < Max);

			if (value < Min || value > Max)
				return false;

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
					bool status = writer.serialize_bits(unsigned_value, bit_count);
					if (!status)
						return false;
				}
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value = static_cast<uint32_t>(value - Min);
				bool status = writer.serialize_bits(unsigned_value, num_bits);
				if (!status)
					return false;
			}

			return true;
		}

		static bool serialize(bit_reader& reader, T& value)
		{
			static_assert(Min < Max);

			constexpr int num_bits = static_cast<int>(utility::bits_in_range(Min, Max));

			static_assert(num_bits <= sizeof(T) * 8);

			if (!reader.can_read_bits(num_bits))
				return false;

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
					bool status = reader.serialize_bits(unsigned_value, bit_count);
					if (!status)
						return false;

					value |= static_cast<T>(unsigned_value) << shift;
				}

				value += Min;
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value;
				bool status = reader.serialize_bits(unsigned_value, num_bits);
				if (!status)
					return false;

				value = static_cast<T>(unsigned_value) + Min;
			}

			if (value < Min || value > Max)
				return false;

			return true;
		}
	};
#pragma endregion
}