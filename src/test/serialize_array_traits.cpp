#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/array_traits.h>
#include <bitstream/traits/bool_trait.h>
#include <bitstream/traits/integral_traits.h>

namespace bitstream::test::traits
{
	BS_ADD_TEST(test_serialize_array_subset)
	{
		using trait = array_subset<uint32_t, bounded_int<uint32_t, 0U, 2048U>>;

		// Test array subset
		uint32_t values_in[6]
		{
			10,
			21,
			42,
			99,
			420,
			1337
		};

		auto compare = [](uint32_t value) { return value != 21 && value != 42 && value != 99; };

		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<trait>(values_in, 6, compare)); // Use bounded_int for writing
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), == , 6);


		uint32_t values_out[6];
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<array_subset<uint32_t>>(values_out, 6, 0U, 2048U)); // Use min, max arguments for reading

		for (int i = 0; i < 6; i++)
		{
			if (!compare(values_in[i]))
				continue;

			BS_TEST_ASSERT(values_out[i] == values_in[i]);
		}
	}
}