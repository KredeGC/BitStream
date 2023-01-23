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
	class bit_writer
	{
	public:
		enum { writing = 1 };
		enum { reading = 0 };

		uint32_t* buffer;
		uint32_t num_bits_written;

		bit_writer()
		{
			this->buffer = nullptr;
			this->scratch = 0;
			this->scratch_bits = 0;
			this->word_index = 0;
			this->num_bits_written = 0;
		}

		bit_writer(void* bytes)
		{
			// TODO: Use an allocator instead of passing in a pointer

			this->buffer = (uint32_t*)bytes;
			this->scratch = 0;
			this->scratch_bits = 0;
			this->word_index = 0;
			this->num_bits_written = 0;
		}

		uint16_t flush()
		{
			if (scratch_bits > 0) {
				uint32_t* ptr = buffer + word_index;
				uint32_t ptr_value = static_cast<uint32_t>(scratch >> 32);
				*ptr = utility::endian_swap_32(ptr_value);

				scratch = 0;
				scratch_bits = 0;
				word_index++;
			}

			return (num_bits_written - 1) / 8 + 1;
		}

		// TODO: Rewrite using allocator
		uint16_t serialize_checksum(uint8_t* protocol_version, uint32_t protocol_size)
		{
			uint16_t num_bytes = flush();

			// Combine the data with version
			uint8_t* combined_data = new uint8_t[(size_t)num_bytes + protocol_size];

			std::memcpy(combined_data, protocol_version, protocol_size);
			std::memcpy(combined_data + protocol_size, buffer, num_bytes);

			// Generate checksum of data + version
			uint32_t checksum = utility::crc_uint32(combined_data, num_bytes + protocol_size);

			std::memmove(buffer + 1, buffer, num_bytes);
			std::memcpy(buffer, &checksum, 4);

			delete[] combined_data;

			return num_bytes + 4;
		}

		bool pad_to_size(uint32_t size)
		{
			flush();

			if (size * 8 < num_bits_written)
				return false;

			num_bits_written = size * 8;
			word_index = (num_bits_written - 1) / 32 + 1; // Might be wrong?

			return true;
		}

		bool align()
		{
			const uint32_t remainder = scratch_bits % 8;
			if (remainder != 0)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, 8 - remainder);

				return status && num_bits_written % 8 == 0;
			}
			return true;
		}

		bool serialize_bits(uint32_t& value, uint32_t num_bits)
		{
			BS_ASSERT_RETURN(num_bits > 0 && num_bits <= 32);

			uint32_t offset = 64 - num_bits - scratch_bits;

			uint64_t ls_value = static_cast<uint64_t>(value) << offset;

			scratch |= ls_value;
			scratch_bits += num_bits;
			num_bits_written += num_bits;

			if (scratch_bits >= 32) {
				uint32_t* ptr = buffer + word_index;
				uint32_t ptr_value = static_cast<uint32_t>(scratch >> 32);
				*ptr = utility::endian_swap_32(ptr_value);
				scratch <<= 32;
				scratch_bits -= 32;
				word_index++;
			}

			return true;
		}

		template<typename... Args>
		bool serialize(Args&&...)
		{
			static_assert(std::false_type::value, "No serialization specialization found for these arguments");

			return false;
		}

	private:
		uint64_t scratch;
		uint32_t scratch_bits;
		uint32_t word_index;
	};

	template<>
	bool bit_writer::serialize<quantization::bounded_range&, float&>(quantization::bounded_range& range, float& value)
	{
		uint32_t int_value = range.quantize(value);

		return serialize_bits(int_value, range.get_bits_required());
	}
}