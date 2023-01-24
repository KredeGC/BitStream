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
	template<typename, typename = void>
	struct serialize_traits;

	template<typename T, T, T>
	struct const_int;

#pragma region bounded_range
	template<>
	struct serialize_traits<quantization::bounded_range>
	{
		static bool serialize(bit_writer& writer, quantization::bounded_range& range, const float& value)
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
	template<typename T>
	struct serialize_traits<T, typename std::enable_if_t<std::is_integral<T>::value>>
	{
		static bool serialize(bit_writer& writer, const T& value, T min, T max)
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

		static bool deserialize(bit_reader& reader, T& value, T min, T max)
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

		static bool deserialize(bit_reader& reader, T& value)
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

#pragma region const char*
	template<>
	struct serialize_traits<const char*>
	{
		static bool serialize(bit_writer& writer, const char* value, uint32_t max_size)
		{
			uint32_t length = static_cast<uint32_t>(strlen(value));
			if (length >= max_size)
				return false;

			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			if (!writer.can_write_bits(length * 8 + num_bits))
				return false;

			if (!writer.serialize_bits(length, num_bits))
				return false;

			if (!writer.serialize_bytes(reinterpret_cast<const uint8_t*>(value), length))
				return false;

			return true;
		}

		static bool deserialize(bit_reader& reader, char* value, uint32_t max_size)
		{
			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			if (!reader.can_read_bits(num_bits))
				return false;

			uint32_t length;
			if (!reader.serialize_bits(length, num_bits))
				return false;

			if (length >= max_size)
				return false;

			if (length == 0)
			{
				value[0] = '\0';
				return true;
			}

			if (!reader.can_read_bits(length * 8))
				return false;

			if (!reader.serialize_bytes(reinterpret_cast<uint8_t*>(value), length))
				return false;

			value[length] = '\0';

			return true;
		}
	};
#pragma endregion
}