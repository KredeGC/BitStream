#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/smallest_three.h"
#include "../utility/bits.h"

#include "bit_reader.h"
#include "bit_writer.h"

#include <cstdint>
#include <string>
#include <type_traits>

namespace bitstream::stream
{
	template<typename...>
	struct serialize_traits;

#pragma region bounded_range
	template<>
	struct serialize_traits<quantization::bounded_range&, float&, void>
	{
		static bool serialize(bit_writer& writer, quantization::bounded_range& range, float& value)
		{
			if (!writer.can_write_bits(range.get_bits_required()))
				return false;

			uint32_t int_value = range.quantize(value);

			return writer.serialize_bits(int_value, range.get_bits_required());
		}

		static bool deserialize(bit_reader& reader, quantization::bounded_range& range, float& value)
		{
			if (!reader.can_read_bits(range.get_bits_required()))
				return false;

			uint32_t int_value;
			if (!reader.serialize_bits(int_value, range.get_bits_required()))
				return false;

			value = range.dequantize(int_value);

			return true;
		}
	};
#pragma endregion

#pragma region integral types
	template<typename T, typename Min, typename Max>
	struct serialize_traits<T&, Min, Max, typename std::enable_if_t<std::is_integral<T>::value>>
	{
		static bool serialize(bit_writer& writer, T& value, std::decay_t<Min> min, std::decay_t<Max> max)
		{
			BS_ASSERT_RETURN(min < max);

			if (value < min || value > max)
				return false;

			int num_bits = static_cast<int>(utility::bits_in_range(min, max));

			if constexpr (sizeof(T) > 4)
			{
				// If the given range is bigger than a word (32 bits)
				int max_words = (num_bits - 1) / 32 + 1;
				for (int i = 0; i < max_words; i++)
				{
					int bit_count = (std::min)(num_bits - i * 32, 32);
					int shift = i * 32;
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

		static bool deserialize(bit_reader& reader, T& value, std::decay_t<Min> min, std::decay_t<Max> max)
		{
			BS_ASSERT_RETURN(min < max);

			int num_bits = static_cast<int>(utility::bits_in_range(min, max));

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
					int bit_count = (std::min)(num_bits - i * 32, 32);
					bool status = reader.serialize_bits(unsigned_value, bit_count);
					if (!status)
						return false;

					int shift = i * 32;
					value |= (static_cast<T>(unsigned_value) + min) << shift;
				}
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

#pragma region char*
	template<typename T, typename U>
	struct serialize_traits<T, U, void>
	{
		static bool serialize(bit_writer& writer, std::decay_t<T> value, U max_size)
		{
			uint32_t length = static_cast<uint32_t>(strlen(value));
			if (length >= max_size - 1)
				return false;

			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			if (!writer.can_write_bits(length * 8 + num_bits))
				return false;

			if (!writer.serialize_bits(length, num_bits))
				return false;

			for (uint32_t i = 0; i < length; i++)
			{
				uint32_t byte_value = static_cast<uint32_t>(value[i]);
				if (!writer.serialize_bits(byte_value, 8))
					return false;
			}

			return true;
		}

		static bool deserialize(bit_reader& reader, std::decay_t<T> value, U max_size)
		{
			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			if (!reader.can_read_bits(num_bits))
				return false;

			uint32_t length;
			if (!reader.serialize_bits(length, num_bits))
				return false;

			if (length >= max_size - 1)
				return false;

			if (length == 0)
			{
				value[0] = '\0';
				return true;
			}

			if (!reader.can_read_bits(length * 8))
				return false;

			for (uint32_t i = 0; i < length; i++)
			{
				uint32_t byte_value;
				if (!reader.serialize_bits(byte_value, 8))
					return false;
				value[i] = byte_value;
			}

			value[length] = '\0';

			return true;
		}
	};
#pragma endregion
}