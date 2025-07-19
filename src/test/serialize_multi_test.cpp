#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/integral_traits.h>
#include <bitstream/traits/quantization_traits.h>

namespace bitstream::test::multi_serialize
{
	BS_ADD_TEST(test_serialize_multi)
	{
		// Test serializing multiple values at once
		uint32_t in_value1 = 511;
		float in_value2 = 99.12345f;

		bounded_range range(-1000.0f, 1000.0f, 0.001f);

		// Write some values
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize(
			multi<uint32_t>(in_value1, 328, 611),
			multi<bounded_range>(range, in_value2)
		));

		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT(num_bits == 30);

		// Read the values back and validate
		uint32_t out_value1;
		float out_value2;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize(
			multi<uint32_t>(out_value1, 328U, 611U),
			multi<bounded_range>(range, out_value2)
		));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(std::abs(in_value2 - out_value2) <= range.get_precision());
	}
}