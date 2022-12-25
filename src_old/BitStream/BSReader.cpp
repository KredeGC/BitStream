#include "bspch.h"
#include "Core/Utility.h"
#include "BSReader.h"
#include "Quantization/HalfPrecision.h"

#include <cstring>

namespace BSNet
{
	BSReader::BSReader()
	{
		this->buffer = nullptr;
		this->scratch = 0;
		this->scratch_bits = 0;
		this->word_index = 0;
		this->total_bits = 0;
		this->num_bits_read = 0;
	}

	BSReader::BSReader(void* bytes, uint32_t num_bytes)
	{
		this->buffer = (uint32_t*)bytes;
		this->scratch = 0;
		this->scratch_bits = 0;
		this->word_index = 0;
		this->total_bits = num_bytes * 8;
		this->num_bits_read = 0;
	}

	bool BSReader::can_read_bits(uint32_t num_bits)
	{
		return num_bits_read + num_bits <= total_bits;
	}

	uint16_t BSReader::get_remaining_bits()
	{
		return total_bits - num_bits_read;
	}

	bool BSReader::serialize_checksum(uint8_t* protocol_version, uint32_t protocol_size)
	{
		uint32_t num_bytes = (total_bits - 1) / 8 + 1;
		uint32_t checksum;

		// Read the data
		std::memcpy(&checksum, buffer, Utility::CHECKSUM_SIZE);

		// Combine the data with version
		uint8_t* combined_data = new uint8_t[(size_t)num_bytes - Utility::CHECKSUM_SIZE + protocol_size];

		std::memcpy(combined_data, protocol_version, protocol_size);
		std::memcpy(combined_data + protocol_size, buffer + 1, num_bytes - Utility::CHECKSUM_SIZE);

		// Generate checksum to compare against
		uint32_t generated_checksum = BSNet::Utility::crc_uint32(combined_data, num_bytes - Utility::CHECKSUM_SIZE + protocol_size);

		delete[] combined_data;

		// Advance the reader by the size of the checksum (32 bits / 1 word)
		word_index++;
		num_bits_read += 32;

		// Compare the checksum
		if (generated_checksum == checksum)
			return true;

		return false;
	}

	bool BSReader::pad_to_size(uint32_t size)
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

	bool BSReader::align()
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

	bool BSReader::serialize_bits(uint32_t& value, uint32_t num_bits)
	{
		BS_ASSERT_RETURN(num_bits > 0 && num_bits <= 32);

		if (scratch_bits < num_bits) {
			uint32_t* ptr = buffer + word_index;

			uint64_t ptr_value = (uint64_t)Utility::endian_swap_32(*ptr) << (32 - scratch_bits);
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

	bool BSReader::serialize_bytes(uint8_t* bytes, uint32_t num_bits)
	{
		uint32_t num_bytes = (num_bits - 1) / 8 + 1;
		for (uint32_t i = 0; i < num_bytes; i++)
		{
			uint32_t value;
			if (!serialize_bits(value, (std::min)(num_bits - i * 8, (uint32_t)8)))
				return false;
			*(bytes + i) = value;
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
		//		uint32_t value = 0;
		//		if (!serialize_bits(value, uneven_scratch))
		//			return false;

		//		value = Utility::endian_swap(value, uneven_scratch);
		//		*((uint32_t*)bytes) = (value << scratch_s) >> scratch_s;
		//	}

		//	// memcpy the middle part, minus the head and tail
		//	uint32_t uneven_words = (num_bits - uneven_scratch) / 32;
		//	uint32_t uneven_bits = (num_bits - uneven_scratch) % 32;
		//	uint32_t num_bytes = uneven_words * 4;
		//	std::memcpy(bytes + uneven_scratch / 8, buffer + word_index, num_bytes);

		//	word_index += uneven_words;
		//	num_bits_read += num_bits - uneven_bits - uneven_scratch;

		//	// Serialize the tail, if not even words
		//	if (uneven_bits != 0)
		//	{
		//		uint32_t value = 0;
		//		if (!serialize_bits(value, uneven_bits))
		//			return false;

		//		value = Utility::endian_swap(value, uneven_bits);
		//		// Get the value, which may be split into the first and second word
		//		*((uint32_t*)bytes + uneven_words) = value >> uneven_scratch;
		//		if (scratch_s != 0) // If the scratch isn't an even word, the value is split between words
		//			*((uint32_t*)bytes + uneven_words + 1) |= value << scratch_s;
		//	}

		//	return true;
		//}

		//uint32_t num_words = (num_bits - 1) / 32 + 1;
		//uint32_t odd_bytes = ((num_bits - 1) % 32) / 8 + 1;
		//for (uint32_t i = 0; i < num_words; i++)
		//{
		//	uint32_t* target = (uint32_t*)bytes + i;
		//	uint32_t value = 0;
		//	if (!serialize_bits(value, (std::min)(num_bits - i * 32, (uint32_t)32)))
		//		return false;
		//	if (i == num_words - 1)
		//	{
		//		uint32_t tmp = Utility::endian_swap(value, num_bits);
		//		std::memcpy(target, &tmp, odd_bytes);
		//	}
		//	else
		//	{
		//		*target = Utility::endian_swap_32(value);
		//	}
		//}

		return true;
	}

	bool BSReader::serialize_string(std::string& string, uint32_t max_size)
	{
		uint32_t length;
		if (!serialize_int<uint32_t>(length, 0, max_size))
			return false;

		if (length >= max_size - 1)
			return false;

		if (length == 0)
		{
			string.assign("");
			return true;
		}

		if (!can_read_bits(length * 8))
			return false;

		char* c_str = new char[length];

		if (!serialize_bytes((uint8_t*)c_str, length * 8))
			return false;

		// TODO: Might cause segfault, but I'm not certain
		string.assign(c_str, length);

		delete[] c_str;

		return true;
	}

	bool BSReader::serialize_chars(char* string, uint32_t max_size)
	{
		uint32_t length;
		if (!serialize_int<uint32_t>(length, 0, max_size))
			return false;

		if (length >= max_size - 1)
			return false;

		if (length == 0)
		{
			string[0] = '\0';
			return true;
		}

		if (!can_read_bits(length * 8))
			return false;

		if (!serialize_bytes((uint8_t*)string, length * 8))
			return false;

		string[length] = '\0';

		return true;
	}
	
	bool BSReader::serialize_half(float& value)
	{
		if (!can_read_bits(16))
			return false;

		uint32_t int_value;
		if (!serialize_bits(int_value, 16))
			return false;

		value = Quantization::HalfPrecision::dequantize(int_value);

		return true;
	}

	bool BSReader::serialize_float(float& value)
	{
		union FloatInt
		{
			float float_value;
			uint32_t int_value;
		};

		constexpr uint32_t num_bits = 32;

		if (!can_read_bits(num_bits))
			return false;

		FloatInt tmp;

		if (!serialize_bits(tmp.int_value, num_bits))
			return false;

		value = tmp.float_value;

		return true;
	}

	bool BSReader::serialize_float(const Quantization::BoundedRange& range, float& value)
	{
		if (!can_read_bits(range.bits_required))
			return false;

		uint32_t int_value;
		if (!serialize_bits(int_value, range.bits_required))
			return false;

		value = range.dequantize(int_value);

		return true;
	}

	bool BSReader::serialize_endpoint(IPEndPoint& value)
	{
		if (!can_read_bits(32 + 16))
			return false;
		
		if (!serialize_bits(value.address, 32))
			return false;

		uint32_t port = value.port;
		if (!serialize_bits(port, 16))
			return false;
		
		value.port = (uint16_t)port;
		value.createAddress();

		return true;
	}
}