#include "../assert.h"
#include "../test.h"

#include <bitstream/quantization/bounded_range.h>

namespace bitstream::test::bounded_range
{
	BS_ADD_TEST(test_bounded_range)
	{
		float value_in = 3.141592f;

		quantization::bounded_range range(0.0f, 5.0f, 0.0001f);

		uint32_t quantized_value = range.quantize(value_in);

		float value_out = range.dequantize(quantized_value);

		BS_TEST_ASSERT(std::abs(value_in - value_out) <= range.get_precision());
		BS_TEST_ASSERT(range.get_bits_required() < 32);
	}
}