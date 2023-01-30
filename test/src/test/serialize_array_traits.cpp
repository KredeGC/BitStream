#include "test_assert.h"
#include "test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/array_traits.h>
#include <bitstream/traits/integral_traits.h>

namespace bitstream::test::traits
{
	BS_ADD_TEST(test_serialize_array_subset)
	{
		using trait = array_subset<uint32_t, bounded_int<uint32_t, 0U, 2048U>>;

		// Test half precision float
		uint32_t values_in[5]
		{
			10,
			21,
			42,
			99,
			1337
		};

		uint8_t buffer[16]{ 0 };
		bit_writer writer(buffer, 16);

		auto compare = [](uint32_t value) { return value != 21 && value != 99; };

		BS_TEST_ASSERT(writer.serialize<trait>(values_in, 5U, compare)); // Use bounded_int for writing
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bytes, == , 6);


		uint32_t values_out[5];
		bit_reader reader(std::move(writer));

		BS_TEST_ASSERT(reader.serialize<array_subset<uint32_t>>(values_out, 5U, compare, 0U, 2048U)); // Use min, max arguments for reading

		for (int i = 0; i < 5; i++)
		{
			if (!compare(values_in[i]))
				continue;

			BS_TEST_ASSERT(values_out[i] == values_in[i]);
		}
	}
}