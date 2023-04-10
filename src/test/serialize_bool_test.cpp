#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/bool_trait.h>

namespace bitstream::test::traits
{
	BS_ADD_TEST(test_serialize_bool_aligned)
	{
		// Test bools
		bool value = true;

		// Write a char array, but make sure the word count isn't whole
		byte_buffer<4> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<bool>(value));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, == , 1);

		// Read the bool back and validate
		bool out_value;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<bool>(out_value));

		BS_TEST_ASSERT_OPERATION(out_value, == , value);
	}

	BS_ADD_TEST(test_serialize_bool_misaligned)
	{
		// Test bools
		uint32_t padding = 17;
		bool value = true;

		// Write a char array, but make sure the word count isn't whole
		byte_buffer<4> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 5U));
		BS_TEST_ASSERT(writer.serialize<bool>(value));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, == , 6);

		// Read the bool back and validate
		uint32_t out_padding;
		bool out_value;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 5U));
		BS_TEST_ASSERT(reader.serialize<bool>(out_value));

		BS_TEST_ASSERT_OPERATION(out_padding, == , padding);
		BS_TEST_ASSERT_OPERATION(out_value, == , value);
		//BS_TEST_ASSERT_OPERATION(*reinterpret_cast<uint8_t*>(&out_value), == , 1U);
	}
}