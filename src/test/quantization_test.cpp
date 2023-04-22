#include "../shared/assert.h"
#include "../shared/test.h"
#include "../shared/test_types.h"

#include <bitstream/quantization/bounded_range.h>
#include <bitstream/quantization/half_precision.h>
#include <bitstream/quantization/smallest_three.h>

#include <cmath>
#include <cstddef>

namespace bitstream::test::quantization
{
	BS_ADD_TEST(test_half_precision)
	{
		float value_in = 3.141592f;

		uint16_t quantized_value = half_precision::quantize(value_in);

		float value_out = half_precision::dequantize(quantized_value);

		constexpr float epsilon = 1e-3f;

		BS_TEST_ASSERT_OPERATION(std::abs(value_in - value_out), <=, epsilon);
	}

	BS_ADD_TEST(test_bounded_range)
	{
		float value_in = 3.141592f;

		constexpr bounded_range range(0.0f, 5.0f, 0.0001f);

		uint32_t quantized_value = range.quantize(value_in);

		float value_out = range.dequantize(quantized_value);

		BS_TEST_ASSERT_OPERATION(std::abs(value_in - value_out), <=, range.get_precision());
		BS_TEST_ASSERT_OPERATION(range.get_bits_required(), <, 32);
	}

	BS_ADD_TEST(test_smallest_three)
	{
		quaternion quat_in{ 0.0f, std::sin(2.0f), std::cos(2.0f), 0.0f };

		auto quantized_quat = smallest_three<quaternion, 11>::quantize(quat_in);
		quaternion quat_out = smallest_three<quaternion, 11>::dequantize(quantized_quat);

		float dot = quat_in[0] * quat_out[0] + quat_in[1] * quat_out[1] + quat_in[2] * quat_out[2] + quat_in[3] * quat_out[3];

		if (dot < 0.0f)
			dot *= -1.0f;

		constexpr float epsilon = 1e-5f;

		BS_TEST_ASSERT_OPERATION(dot, >=, (1.0f - epsilon));
	}
}