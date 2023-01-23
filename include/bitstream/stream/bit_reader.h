#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/smallest_three.h"
#include "../utility/crc.h"
#include "../utility/endian.h"

#include <cstdint>
#include <string>
#include <type_traits>

namespace bitstream::stream
{
	class bit_reader
	{
	public:
		static constexpr bool writing = false;
		static constexpr bool reading = true;

		uint32_t* buffer;
		uint32_t num_bits_read;

		bit_reader()
		{
			this->buffer = nullptr;
			this->scratch = 0;
			this->scratch_bits = 0;
			this->word_index = 0;
			this->total_bits = 0;
			this->num_bits_read = 0;
		}

		bit_reader(void* bytes, uint32_t num_bytes)
		{
			this->buffer = (uint32_t*)bytes;
			this->scratch = 0;
			this->scratch_bits = 0;
			this->word_index = 0;
			this->total_bits = num_bytes * 8;
			this->num_bits_read = 0;
		}

		uint32_t get_num_bits_read() { return num_bits_read; }

		bool can_read_bits(uint32_t num_bits) { return num_bits_read + num_bits <= total_bits; }

		uint32_t get_remaining_bits() { return total_bits - num_bits_read; }

		bool serialize_checksum(uint32_t protocol_version)
		{
			uint32_t num_bytes = (total_bits - 1) / 8 + 1;

			// Read the checksum
			uint32_t checksum;
			std::memcpy(&checksum, buffer, sizeof(uint32_t));

			// Copy protocol version to buffer
			std::memcpy(buffer, &protocol_version, sizeof(uint32_t));

			// Generate checksum to compare against
			uint32_t generated_checksum = utility::crc_uint32(reinterpret_cast<uint8_t*>(buffer), num_bytes);

			// Write the checksum back, just in case
			std::memcpy(buffer, &checksum, sizeof(uint32_t));

			// Advance the reader by the size of the checksum (32 bits / 1 word)
			word_index++;
			num_bits_read += 32;

			// Compare the checksum
			return generated_checksum == checksum;
		}

		bool pad_to_size(uint32_t size)
		{
			if (size * 8 > total_bits || size * 8 < num_bits_read)
				return false;

			// Align with word size
			const int remainder = num_bits_read % 32;
			if (remainder != 0)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 32 - remainder);

				if (!status || zero != 0)
					return false;
			}

			// Test for zeros in padding
			for (uint32_t i = word_index; i < size / 4; i++)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, 32);

				if (!status || zero != 0)
					return false;
			}

			return true;
		}

		bool align()
		{
			const uint32_t remainder = num_bits_read % 8;
			if (remainder != 0)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 8 - remainder);

				return status && zero == 0 && num_bits_read % 8 == 0;
			}
			return true;
		}

		bool serialize_bits(uint32_t& value, uint32_t num_bits)
		{
			BS_ASSERT_RETURN(num_bits > 0 && num_bits <= 32);

			if (scratch_bits < num_bits) {
				uint32_t* ptr = buffer + word_index;

				uint64_t ptr_value = (uint64_t)utility::endian_swap_32(*ptr) << (32 - scratch_bits);
				scratch |= ptr_value;
				scratch_bits += 32;
				word_index++;
			}

			uint32_t offset = 64 - num_bits;
			value = (uint32_t)(scratch >> offset);

			scratch <<= num_bits;
			scratch_bits -= num_bits;
			num_bits_read += num_bits;

			return true;
		}

		template<typename T, typename... Args>
		bool serialize(T&&, Args&&...)
		{
			static_assert(std::false_type::value, "No serialization specialization found for these arguments");

			return false;
		}

	private:
		uint64_t scratch;
		uint32_t scratch_bits;
		uint32_t word_index;
		uint32_t total_bits;
	};

	template<>
	bool bit_reader::serialize<quantization::bounded_range&, float&>(quantization::bounded_range& range, float& value)
	{
		if (!can_read_bits(range.get_bits_required()))
			return false;

		uint32_t int_value;
		if (!serialize_bits(int_value, range.get_bits_required()))
			return false;

		value = range.dequantize(int_value);

		return true;
	}
}