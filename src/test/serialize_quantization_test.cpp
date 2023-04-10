#include "../shared/assert.h"
#include "../shared/test.h"
#include "../shared/test_types.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/quantization_traits.h>

#include <cmath>

namespace bitstream::test::traits
{
    BS_ADD_TEST(test_serialize_half_precision)
	{
		// Test half precision float
		float value_in = 3.141592f;

		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<half_precision>(value_in));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, ==, 16);


		float value_out;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<half_precision>(value_out));
        
        constexpr float epsilon = 1e-3f;

		BS_TEST_ASSERT_OPERATION(std::abs(value_in - value_out), <=, epsilon);
	}

	BS_ADD_TEST(test_serialize_bounded_range)
	{
		// Test bounded float
		float value_in = 3.141592f;
		bounded_range range(0.0f, 5.0f, 0.0001f);

		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<bounded_range>(range, value_in));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, == , 16);


		float value_out;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<bounded_range>(range, value_out));

		BS_TEST_ASSERT_OPERATION(std::abs(value_in - value_out), <= , range.get_precision());
		BS_TEST_ASSERT_OPERATION(range.get_bits_required(), < , 32);
	}

	BS_ADD_TEST(test_serialize_smallest_three)
	{
		using trait = smallest_three<quaternion, 11>;

		// Test smallest three
		quaternion value_in{ 0.0f, std::sin(2.0f), std::cos(2.0f), 0.0f };

		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<trait>(value_in));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, == , 11 * 3 + 2);


		quaternion value_out;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<trait>(value_out));

		float dot = value_in[0] * value_out[0] + value_in[1] * value_out[1] + value_in[2] * value_out[2] + value_in[3] * value_out[3];

		if (dot < 0.0f)
			dot *= -1.0f;

		constexpr float epsilon = 1e-5f;

		BS_TEST_ASSERT_OPERATION(dot, >= , (1.0f - epsilon));
	}
}