#include "../assert.h"
#include "../test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>
#include <bitstream/stream/serialize_traits.h>

namespace bitstream::test::serialization
{
	BS_ADD_TEST(test_serialize_bits)
	{
		// Test serializing bits
		uint32_t in_value1 = 511;
		uint32_t in_value2 = 99;
		uint32_t in_value3 = 1111;

		constexpr uint32_t bits_required = utility::bits_to_represent(1111);

		// Write some values with a few bits
		uint8_t buffer[8]{ 0 };
		stream::bit_writer writer(buffer, 8);

		BS_TEST_ASSERT(writer.serialize_bits(in_value1, bits_required));
		BS_TEST_ASSERT(writer.serialize_bits(in_value2, bits_required));
		BS_TEST_ASSERT(writer.serialize_bits(in_value3, bits_required));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 5);

		// Read the values back and validate
		uint32_t out_value1;
		uint32_t out_value2;
		uint32_t out_value3;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, bits_required));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, bits_required));
		BS_TEST_ASSERT(reader.serialize_bits(out_value3, bits_required));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(out_value2 == in_value2);
		BS_TEST_ASSERT(out_value3 == in_value3);
	}

	BS_ADD_TEST(test_serialize_uint8)
	{
		// Test unsigned int8
		uint8_t value = 98;

		uint8_t buffer[16]{ 0 };
		stream::bit_writer writer(buffer, 16);
		BS_TEST_ASSERT(writer.serialize<uint8_t>(value, 20U, 127U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes <= sizeof(uint8_t));


		uint8_t out_value = 0;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<uint8_t>(out_value, 20U, 127U));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_uint16)
	{
		using trait_type = stream::const_int<uint16_t, 20U, 400U>;

		// Test unsigned int16
		uint16_t value = 131;

		uint8_t buffer[16];
		stream::bit_writer writer(buffer, 16);
		BS_TEST_ASSERT(writer.serialize<trait_type>(value));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes <= sizeof(uint16_t));


		uint16_t out_value = 0;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<trait_type>(out_value));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_uint32)
	{
		// Test unsigned int32
		uint32_t value = 131;

		uint8_t buffer[16];
		stream::bit_writer writer(buffer, 16);
		BS_TEST_ASSERT(writer.serialize<uint32_t>(value, 20U, 400U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes <= sizeof(uint32_t));


		uint32_t out_value = 0;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<uint32_t>(out_value, 20U, 400U));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_uint64)
	{
		// Test unsigned int64
		uint64_t value = 1099511627776ULL;

		uint8_t buffer[16];
		stream::bit_writer writer(buffer, 16);
		BS_TEST_ASSERT(writer.serialize<uint64_t>(value, 80ULL, 4398046511104ULL));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes <= sizeof(uint64_t));


		uint64_t out_value = 0;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<uint64_t>(out_value, 80ULL, 4398046511104ULL));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_uint64_const_large)
	{
		using trait_type = stream::const_int<uint64_t, 80ULL, 4398046511104ULL>;

		// Test unsigned int64
		uint64_t value = 1099511627776ULL;

		uint8_t buffer[16];
		stream::bit_writer writer(buffer, 16);
		BS_TEST_ASSERT(writer.serialize<trait_type>(value));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes <= sizeof(uint64_t));


		uint64_t out_value = 0;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<trait_type>(out_value));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_uint64_const_small)
	{
		using trait_type = stream::const_int<uint64_t, 80ULL, 2147483648ULL>;

		// Test unsigned int64
		uint64_t value = 1073741824ULL;

		uint8_t buffer[16];
		stream::bit_writer writer(buffer, 16);
		BS_TEST_ASSERT(writer.serialize<trait_type>(value));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes <= sizeof(uint64_t));


		uint64_t out_value = 0;
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<trait_type>(out_value));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_chars_aligned)
	{
		// Test c-style strings
		uint32_t padding = 233;
		const char* value = "Hello, world!";

		// Write a char array, but make sure the word count isn't whole
		uint8_t buffer[32];
		stream::bit_writer writer(buffer, 32);
		BS_TEST_ASSERT(writer.serialize_bits(padding, 8));
		BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 15);

		// Read the array back and validate
		uint32_t out_padding = 0;
		char out_value[32];
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 8));
		BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U));

		BS_TEST_ASSERT(out_padding == padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}

	BS_ADD_TEST(test_serialize_chars_misaligned)
	{
		// Test c-style strings
		uint32_t padding = 42;
		const char* value = "Hello, world!";

		// Write a char array, but with an uneven bit offset
		uint8_t buffer[32];
		stream::bit_writer writer(buffer, 32);
		BS_TEST_ASSERT(writer.serialize_bits(padding, 6));
		BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 15);

		// Read the array back and validate
		uint32_t out_padding = 0;
		char out_value[32];
		stream::bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 6));
		BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U));

		BS_TEST_ASSERT(out_padding == padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}
}