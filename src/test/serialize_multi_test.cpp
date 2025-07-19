#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/integral_traits.h>

namespace bitstream::test::multi_serialize
{
	BS_ADD_TEST(test_serialize_multi)
	{
		// Test serializing multiple values at once
		uint32_t in_value1 = 511;
		uint32_t in_value2 = 99;

		// Write some values
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize(
			multi<uint32_t>(in_value1, 328, 611),
			multi<uint32_t>(in_value2, 11, 111)
		));

		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT(num_bits == 16);

		// Read the values back and validate
		uint32_t out_value1;
		uint32_t out_value2;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize(
			multi<uint32_t>(out_value1, 328U, 611U),
			multi<uint32_t>(out_value2, 11U, 111U)
		));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(out_value2 == in_value2);
	}
}