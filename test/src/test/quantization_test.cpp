#include "test_assert.h"
#include "test.h"

#include <bitstream/quantization/bounded_range.h>
#include <bitstream/quantization/half_precision.h>
#include <bitstream/quantization/smallest_three.h>

#include <cmath>

namespace bitstream::test::quantization
{
	struct quaternion
	{
		float w;
		float x;
		float y;
		float z;

		quaternion() = default;

		quaternion(float w, float x, float y, float z)
			: w(w), x(x), y(y), z(z) {}
	};

	BS_ADD_TEST(test_half_precision)
	{
		float value_in = 3.141592f;

		uint16_t quantized_value = half_precision::quantize(value_in);

		float value_out = half_precision::dequantize(quantized_value);

		constexpr float epsilon = 1e-3f;

		BS_TEST_ASSERT(std::abs(value_in - value_out) <= epsilon);
	}

	BS_ADD_TEST(test_bounded_range)
	{
		float value_in = 3.141592f;

		bounded_range range(0.0f, 5.0f, 0.0001f);

		uint32_t quantized_value = range.quantize(value_in);

		float value_out = range.dequantize(quantized_value);

		BS_TEST_ASSERT(std::abs(value_in - value_out) <= range.get_precision());
		BS_TEST_ASSERT(range.get_bits_required() < 32);
	}

	BS_ADD_TEST(test_smallest_three)
	{
		quaternion quat_in(0.0f, std::sin(2.0f), std::cos(2.0f), 0.0f);

		auto quantized_quat = smallest_three<quaternion, 11>::quantize(quat_in);
		quaternion quat_out = smallest_three<quaternion, 11>::dequantize(quantized_quat);

		float dot = quat_in.x * quat_out.x + quat_in.y * quat_out.y + quat_in.z * quat_out.z + quat_in.w * quat_out.w;

		if (dot < 0.0f)
			dot *= -1.0f;

		constexpr float epsilon = 1e-5f;

		BS_TEST_ASSERT(dot >= (1.0f - epsilon));
	}
}