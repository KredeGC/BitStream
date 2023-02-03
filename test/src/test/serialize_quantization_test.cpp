#include "test_assert.h"
#include "test.h"
#include "test_types.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/quantization_traits.h>

#include <cmath>

namespace bitstream::test::traits
{
	BS_ADD_TEST(test_serialize_float)
	{
		// Test float
		float value_in = 3.141592f;

		uint8_t buffer[16]{ 0 };
		bit_writer writer(buffer, 16);

		BS_TEST_ASSERT(writer.serialize<float>(value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 4);


		float value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<float>(value_out));

		BS_TEST_ASSERT_OPERATION(value_in, ==, value_out);
	}

    BS_ADD_TEST(test_serialize_half_precision)
	{
		// Test half precision float
		float value_in = 3.141592f;

		uint8_t buffer[16]{ 0 };
		bit_writer writer(buffer, 16);

		BS_TEST_ASSERT(writer.serialize<half_precision>(value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, ==, 2);


		float value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<half_precision>(value_out));
        
        constexpr float epsilon = 1e-3f;

		BS_TEST_ASSERT_OPERATION(std::abs(value_in - value_out), <=, epsilon);
	}

	BS_ADD_TEST(test_serialize_bounded_range)
	{
		// Test bounded float
		float value_in = 3.141592f;
		bounded_range range(0.0f, 5.0f, 0.0001f);

		uint8_t buffer[16]{ 0 };
		bit_writer writer(buffer, 16);

		BS_TEST_ASSERT(writer.serialize<bounded_range>(range, value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 2);


		float value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<bounded_range>(range, value_out));

		BS_TEST_ASSERT_OPERATION(std::abs(value_in - value_out), <= , range.get_precision());
		BS_TEST_ASSERT_OPERATION(range.get_bits_required(), < , 32);
	}

	BS_ADD_TEST(test_serialize_smallest_three)
	{
		using trait = smallest_three<quaternion, 11>;

		// Test smallest three
		quaternion value_in(0.0f, std::sin(2.0f), std::cos(2.0f), 0.0f);

		uint8_t buffer[16]{ 0 };
		bit_writer writer(buffer, 16);

		BS_TEST_ASSERT(writer.serialize<trait>(value_in));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 5);


		quaternion value_out;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<trait>(value_out));

		float dot = value_in.x * value_out.x + value_in.y * value_out.y + value_in.z * value_out.z + value_in.w * value_out.w;

		if (dot < 0.0f)
			dot *= -1.0f;

		constexpr float epsilon = 1e-5f;

		BS_TEST_ASSERT_OPERATION(dot, >= , (1.0f - epsilon));
	}
}