#include "../shared/assert.h"
#include "../shared/test.h"
#include "../shared/test_types.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/enum_trait.h>

namespace bitstream::test::traits
{
	BS_ADD_TEST(test_serialize_enum_aligned)
	{
		// Test enums
		test_enum value = test_enum::SecondValue;

		// Write a char array, but make sure the word count isn't whole
		byte_buffer<4> buffer;
		bit_writer writer(buffer);
        
		BS_TEST_ASSERT(writer.serialize<test_enum>(value, 1, 3));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 1);

		// Read the enum back and validate
		test_enum out_value;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<test_enum>(out_value, 1, 3));

		BS_TEST_ASSERT_OPERATION(out_value, == , value);
	}
}