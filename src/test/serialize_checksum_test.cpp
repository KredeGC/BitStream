#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/checksum_trait.h>

namespace bitstream::test::traits
{
	// Checksums are stored in the first 4 bytes of the buffer
	// They should only be used when done writing since they flush the buffer
	// It is important to serialize the checksum both at the start and end when writing
	// When reading you only need to serialize at the start as the end is a noop

	BS_ADD_TEST(test_serialize_checksum_empty)
	{
		// Test checksum
		using protocol_version = checksum<0xBEEFCAFE>;

		// Write a checksum
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<protocol_version>());
		BS_TEST_ASSERT(writer.serialize<protocol_version>());
		uint32_t num_bits = writer.flush();

		// Read the checksum and validate
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<protocol_version>());
		// When reading the last checksum check does nothing
	}

	BS_ADD_TEST(test_serialize_checksum)
	{
		// Test checksum
		using protocol_version = checksum<0xDEADBEEF>;
		uint32_t value = 5;

		// Write some initial values and finish with a checksum
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<protocol_version>()); // Must be called both before and after
		BS_TEST_ASSERT(writer.serialize_bits(value, 3));
		BS_TEST_ASSERT(writer.serialize<protocol_version>());
		uint32_t num_bits = writer.flush();

		// Read the checksum and validate
		uint32_t out_value;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<protocol_version>());
		BS_TEST_ASSERT(reader.serialize_bits(out_value, 3));
		BS_TEST_ASSERT(reader.serialize<protocol_version>()); // When reading the last checksum check does nothing

		BS_TEST_ASSERT(out_value == value);
	}
}