#include "test_assert.h"
#include "test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/quantization_traits.h>

namespace bitstream::test::quantization
{
    BS_ADD_TEST(test_serialize_half_precision)
	{
		// Test half precision float
		float value_in = 3.141592f;

		uint8_t buffer[16]{ 0 };
		bit_writer writer(buffer, 16);

		BS_TEST_ASSERT(writer.serialize<half_precision>(value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 2);


		float value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<half_precision>(value_out));
        
        constexpr float epsilon = 1e-3f;

		BS_TEST_ASSERT(std::abs(value_in - value_out) <= epsilon);
	}
}