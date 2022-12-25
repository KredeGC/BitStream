#include "Core/Utility.h"
#include "BSWriter.h"
#include "HalfPrecision.h"

#include <cstring>

namespace BSNet
{
	BSWriter::BSWriter()
	{
		this->buffer = nullptr;
		this->scratch = 0;
		this->scratch_bits = 0;
		this->word_index = 0;
		this->num_bits_written = 0;
	}

	BSWriter::BSWriter(void* bytes)
	{
		this->buffer = (uint32_t*)bytes;
		this->scratch = 0;
		this->scratch_bits = 0;
		this->word_index = 0;
		this->num_bits_written = 0;
	}

	uint16_t BSWriter::flush()
	{
		if (scratch_bits > 0) {
			uint32_t* ptr = buffer + word_index;
			uint32_t ptr_value = (uint32_t)(scratch >> 32);
			*ptr = Utility::endian_swap_32(ptr_value);

			scratch = 0;
			scratch_bits = 0;
			word_index++;
		}

		return (num_bits_written - 1) / 8 + 1;
	}

	uint16_t BSWriter::serialize_checksum(uint8_t* protocol_version, uint32_t protocol_size)
	{
		uint16_t num_bytes = flush();

		// Combine the data with version
		uint8_t* combined_data = new uint8_t[(size_t)num_bytes + protocol_size];

		std::memcpy(combined_data, protocol_version, protocol_size);
		std::memcpy(combined_data + protocol_size, buffer, num_bytes);

		// Generate checksum of data + version
		uint32_t checksum = BSNet::Utility::crc_uint32(combined_data, num_bytes + protocol_size);

		std::memmove(buffer + 1, buffer, num_bytes);
		std::memcpy(buffer, &checksum, 4);

		delete[] combined_data;

		return num_bytes + 4;
	}

	bool BSWriter::pad_to_size(uint32_t size)
	{
		flush();

		if (size * 8 < num_bits_written)
			return false;

		//uint32_t padding = size * 8 - num_bits_written;

		num_bits_written = size * 8;
		word_index = (num_bits_written - 1) / 32 + 1; // Might be wrong?

		return true;
	}

	bool BSWriter::align()
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

	bool BSWriter::serialize_bits(uint32_t& value, uint32_t num_bits)
	{
		BS_ASSERT_RETURN(num_bits > 0 && num_bits <= 32);

		uint32_t offset = 64 - num_bits - scratch_bits;

		uint64_t ls_value = (uint64_t)value << offset;

		scratch |= ls_value;
		scratch_bits += num_bits;
		num_bits_written += num_bits;

		if (scratch_bits >= 32) {
			uint32_t* ptr = buffer + word_index;
			uint32_t ptr_value = (uint32_t)(scratch >> 32);
			*ptr = Utility::endian_swap_32(ptr_value);
			scratch <<= 32;
			scratch_bits -= 32;
			word_index++;
		}

		return true;
	}

	bool BSWriter::serialize_bytes(uint8_t* bytes, uint32_t num_bits)
	{
		// Should this be here? Or should the responsibility be elsewhere?
		if (num_bits == 0)
			return false;

		/*if (scratch_bits % 8 == 0)
		{
			uint32_t num_words = num_bits / 32;

			uint32_t num_scratch_bytes = scratch_bits / 8;
			if (scratch_bits != 0)
			{
				uint32_t value = (uint32_t)(scratch >> 32);
				std::memcpy(buffer + word_index, &value, num_scratch_bytes);

				scratch = 0;
				scratch_bits = 0;
			}

			uint32_t num_bytes = (num_bits - 1) / 8 + 1;
			uint32_t num_mid_bytes = num_bytes - num_scratch_bytes;

			std::memcpy((uint8_t*)buffer + word_index * 4 + num_scratch_bytes, bytes, num_mid_bytes);

			num_bits_written += num_bits;
			word_index += num_words;
			scratch_bits = (num_bits + num_scratch_bytes * 8) % 32;

			uint32_t remaining_bytes = (num_bytes + num_scratch_bytes) % 4;
			if (remaining_bytes != 0)
			{
				uint32_t offset = 64 - scratch_bits;

				uint64_t value = 0;
				for (uint32_t i = 0; i < remaining_bytes; i++)
				{
					uint64_t tmp = ((uint64_t) * (bytes + num_bytes - i - 1));
					value |= tmp << (i * 8);
				}

				scratch = value << offset;
			}

			return true;
		}*/

		uint32_t num_bytes = (num_bits - 1) / 8 + 1;
		for (uint32_t i = 0; i < num_bytes; i++)
		{
			uint32_t value = (uint32_t)*(bytes + i);
			if (!serialize_bits(value, (std::min)(num_bits - i * 8, (uint32_t)8)))
		 		return false;
		}

		// TODO: FIXME, unused optimizations
		//if (scratch_bits % 8 == 0)
		//{
		//	uint32_t scratch_s = scratch_bits;
		//	uint32_t uneven_scratch = (32 - scratch_s) % 32;

		//	// Serialize the head normally, if not even words
		//	if (scratch_s != 0)
		//	{
		//		// Get the value, but masked to only the first few bits
		//		uint32_t value = (*((uint32_t*)bytes) << scratch_s) >> scratch_s;
		//		value = Utility::endian_swap(value, uneven_scratch);
		//		if (!serialize_bits(value, uneven_scratch))
		//			return false;
		//	}

		//	// memcpy the middle part, minus the head and tail
		//	uint32_t uneven_words = (num_bits - uneven_scratch) / 32;
		//	uint32_t uneven_bits = (num_bits - uneven_scratch) % 32;
		//	uint32_t num_bytes = uneven_words * 4;
		//	std::memcpy(buffer + word_index, bytes + uneven_scratch / 8, num_bytes);

		//	word_index += uneven_words;
		//	num_bits_written += num_bits - uneven_bits - uneven_scratch;

		//	// Serialize the tail, if not even words
		//	if (uneven_bits != 0)
		//	{
		//		// Get the value, which may be split into the first and second word
		//		uint32_t value = *((uint32_t*)bytes + uneven_words) >> uneven_scratch;
		//		if (scratch_s != 0) // If the scratch isn't an even word, the value is split between words
		//			value |= *((uint32_t*)bytes + uneven_words + 1) << scratch_s;

		//		value = Utility::endian_swap(value, uneven_bits);
		//		if (!serialize_bits(value, uneven_bits))
		//			return false;
		//	}

		//	return true;
		//}

		//// If the word isn't even, then we can't memcpy :(
		//uint32_t num_words = (num_bits - 1) / 32 + 1;
		//for (uint32_t i = 0; i < num_words; i++)
		//{
		//	uint32_t value = *((uint32_t*)bytes + i);
		//	if (i == num_words - 1)
		//		value = Utility::endian_swap(value, num_bits);
		//	else
		//		value = Utility::endian_swap_32(value);
		//	if (!serialize_bits(value, (std::min)(num_bits - i * 32, (uint32_t)32)))
		//		return false;
		//}

		return true;
	}

	bool BSWriter::serialize_string(std::string& string, uint32_t max_size)
	{
		uint32_t length = (uint32_t)string.size();
		if (length >= max_size - 1)
			return false;

		if (!serialize_int<uint32_t>(length, 0, max_size))
			return false;

		if (length == 0)
			return true;

		return serialize_bytes((uint8_t*)string.c_str(), length * 8);
	}

	bool BSWriter::serialize_chars(char* string, uint32_t max_size)
	{
		uint32_t length = (uint32_t)strlen(string);
		if (length >= max_size - 1)
			return false;

		if (!serialize_int<uint32_t>(length, 0, max_size))
			return false;

		return serialize_bytes((uint8_t*)string, length * 8);
	}
	
	bool BSWriter::serialize_half(float& value)
	{
		uint32_t int_value = Quantization::HalfPrecision::quantize(value);

		return serialize_bits(int_value, 16);
	}

	bool BSWriter::serialize_float(float& value)
	{
		union FloatInt
		{
			float float_value;
			uint32_t int_value;
		};

		constexpr uint32_t num_bits = 32;

		FloatInt tmp;
		tmp.float_value = value;

		return serialize_bits(tmp.int_value, num_bits);
	}

	bool BSWriter::serialize_float(const Quantization::BoundedRange& range, float& value)
	{
		uint32_t int_value = range.quantize(value);

		return serialize_bits(int_value, range.bits_required);
	}

	bool BSWriter::serialize_endpoint(IPEndPoint& value)
	{
		if (!serialize_bits(value.address, 32))
			return false;

		uint32_t port = value.port;
		return serialize_bits(port, 16);
	}
}