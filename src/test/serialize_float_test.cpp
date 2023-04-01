#include "../shared/assert.h"
#include "../shared/test.h"
#include "../shared/test_types.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/float_trait.h>

namespace bitstream::test::traits
{
	BS_ADD_TEST(test_serialize_float)
	{
		// Test float
		float value_in = 3.141592f;

		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<float>(value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 4);


		float value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<float>(value_out));

		BS_TEST_ASSERT_OPERATION(value_in, == , value_out);
	}

	BS_ADD_TEST(test_serialize_double)
	{
		// Test float
		double value_in = 3.141592;

		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<double>(value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 8);


		double value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<double>(value_out));

		BS_TEST_ASSERT_OPERATION(value_in, == , value_out);
	}
}