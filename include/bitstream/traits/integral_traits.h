#pragma once
#include "../utility/assert.h"
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace bitstream
{
	/**
	 * @brief Wrapper type for compile-time known integer bounds
	 * @tparam T 
	*/
	template<typename T, T, T>
	struct bounded_int;

#pragma region integral types
	/**
	 * @brief A trait used to serialize integer values with runtime bounds
	 * @tparam T A type matching an integer value
	*/
	template<typename T>
	struct serialize_traits<T, typename std::enable_if_t<std::is_integral_v<T>>>
	{
		static bool serialize(bit_writer& writer, T value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)()) noexcept
		{
			BS_ASSERT(min < max);
            
            BS_ASSERT(value >= min && value <= max);

			uint32_t num_bits = utility::bits_in_range(min, max);

			BS_ASSERT(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4)
			{
				if (num_bits > 32)
				{
					// If the given range is bigger than a word (32 bits)
					uint32_t unsigned_value = static_cast<uint32_t>(value - min);
					BS_ASSERT(writer.serialize_bits(unsigned_value, 32));

					unsigned_value = static_cast<uint32_t>((value - min) >> 32);
					BS_ASSERT(writer.serialize_bits(unsigned_value, num_bits - 32));

					return true;
				}
			}

			// If the given range is smaller than or equal to a word (32 bits)
			uint32_t unsigned_value = static_cast<uint32_t>(value - min);
			BS_ASSERT(writer.serialize_bits(unsigned_value, num_bits));

			return true;
		}

		static bool serialize(bit_reader& reader, T& value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)()) noexcept
		{
			BS_ASSERT(min < max);

			uint32_t num_bits = utility::bits_in_range(min, max);

			BS_ASSERT(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4)
			{
				if (num_bits > 32)
				{
					// If the given range is bigger than a word (32 bits)
					value = 0;
					uint32_t unsigned_value;

					BS_ASSERT(reader.serialize_bits(unsigned_value, 32));
					value |= static_cast<T>(unsigned_value);

					BS_ASSERT(reader.serialize_bits(unsigned_value, num_bits - 32));
					value |= static_cast<T>(unsigned_value) << 32;

					value += min;

					BS_ASSERT(value >= min && value <= max);

					return true;
				}
			}

			// If the given range is smaller than or equal to a word (32 bits)
			uint32_t unsigned_value;
			BS_ASSERT(reader.serialize_bits(unsigned_value, num_bits));

			value = static_cast<T>(unsigned_value) + min;
            
            BS_ASSERT(value >= min && value <= max);

			return true;
		}
	};
#pragma endregion

#pragma region const integral types
	/**
	 * @brief A trait used to serialize integer values with compiletime bounds
	 * @tparam T A type matching an integer value
	 * @tparam Min The lower bound
	 * @tparam Max The upper bound
	*/
	template<typename T, T Min, T Max>
	struct serialize_traits<bounded_int<T, Min, Max>, typename std::enable_if_t<std::is_integral_v<T>>>
	{
		static bool serialize(bit_writer& writer, T value) noexcept
		{
			static_assert(Min < Max);
            
            BS_ASSERT(value >= Min && value <= Max);

			constexpr uint32_t num_bits = utility::bits_in_range(Min, Max);

			static_assert(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4 && num_bits > 32)
			{
				// If the given range is bigger than a word (32 bits)
				uint32_t unsigned_value = static_cast<uint32_t>(value - Min);
				BS_ASSERT(writer.serialize_bits(unsigned_value, 32));

				unsigned_value = static_cast<uint32_t>((value - Min) >> 32);
				BS_ASSERT(writer.serialize_bits(unsigned_value, num_bits - 32));
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

			constexpr uint32_t num_bits = utility::bits_in_range(Min, Max);

			static_assert(num_bits <= sizeof(T) * 8);

			if constexpr (sizeof(T) > 4 && num_bits > 32)
			{
				// If the given range is bigger than a word (32 bits)
				value = 0;
				uint32_t unsigned_value;

				BS_ASSERT(reader.serialize_bits(unsigned_value, 32));
				value |= static_cast<T>(unsigned_value);

				BS_ASSERT(reader.serialize_bits(unsigned_value, num_bits - 32));
				value |= static_cast<T>(unsigned_value) << 32;

				value += Min;
			}
			else
			{
				// If the given range is smaller than or equal to a word (32 bits)
				uint32_t unsigned_value;
				BS_ASSERT(reader.serialize_bits(unsigned_value, num_bits));

				value = static_cast<T>(unsigned_value) + Min;
			}
            
            BS_ASSERT(value >= Min && value <= Max);

			return true;
		}
	};
#pragma endregion
}