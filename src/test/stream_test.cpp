#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>
#include <bitstream/utility/bits.h>

namespace bitstream::test::stream
{
    BS_ADD_TEST(test_serialize_fail)
    {
        // Test serializing failure
		uint32_t in_value1 = 511;
		uint32_t in_value2 = 99;
        
		// Write some values with too many bits
		byte_buffer<4> buffer;
		bit_writer writer(buffer);
        
		BS_TEST_ASSERT(writer.serialize_bits(in_value1, 11));
		BS_TEST_ASSERT(writer.serialize_bits(in_value2, 11));
        BS_TEST_ASSERT(!writer.can_serialize_bits(11)); // Assert that there's no more space
		uint32_t num_bytes = writer.flush();
        
		BS_TEST_ASSERT(num_bytes == 3);

		// Read the values back and validate
		uint32_t out_value1;
		uint32_t out_value2;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, 11));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, 11));
		BS_TEST_ASSERT(!reader.can_serialize_bits(11));
        BS_TEST_ASSERT(reader.get_remaining_bits() < 8);

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(out_value2 == in_value2);
    }
    
	BS_ADD_TEST(test_serialize_bits)
	{
		// Test serializing bits
		uint32_t in_value1 = 511;
		uint32_t in_value2 = 99;
		uint32_t in_value3 = 1111;

		constexpr uint32_t bits_required = utility::bits_to_represent(1111);

		// Write some values with a few bits
		byte_buffer<8> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(in_value1, bits_required));
		BS_TEST_ASSERT(writer.serialize_bits(in_value2, bits_required));
		BS_TEST_ASSERT(writer.serialize_bits(in_value3, bits_required));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 5);

		// Read the values back and validate
		uint32_t out_value1;
		uint32_t out_value2;
		uint32_t out_value3;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, bits_required));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, bits_required));
		BS_TEST_ASSERT(reader.serialize_bits(out_value3, bits_required));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(out_value2 == in_value2);
		BS_TEST_ASSERT(out_value3 == in_value3);
	}

	BS_ADD_TEST(test_serialize_checksum)
	{
		// Test checksum
		uint32_t protocol_version = 0xDEADBEEF;
		uint32_t value = 5;

		// Write some initial values and finish with a checksum
		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		writer.prepend_checksum();
		BS_TEST_ASSERT(writer.serialize_bits(value, 3));
		uint16_t num_bytes = writer.serialize_checksum(protocol_version);

		// Read the checksum and validate
		uint32_t out_value;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_checksum(protocol_version));
		BS_TEST_ASSERT(reader.serialize_bits(out_value, 3));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_padding_small)
	{
		// Test padding
		uint32_t in_value1 = 3;
		uint32_t in_value2 = 6;

		// Write some initial value, pad it, and then write another value
		byte_buffer<32> buffer;
		bit_writer writer(buffer);
		BS_TEST_ASSERT(writer.serialize_bits(in_value1, 3));
		BS_TEST_ASSERT(writer.pad_to_size(2));
		BS_TEST_ASSERT(writer.serialize_bits(in_value2, 5));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 3);

		// Read the values and validate padding
		uint32_t out_value1;
		uint32_t out_value2;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, 3));
		BS_TEST_ASSERT(reader.pad_to_size(2));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, 5));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(out_value2 == in_value2);
	}

	BS_ADD_TEST(test_serialize_padding_large)
	{
		// Test padding
		uint32_t in_value1 = 3;
		uint32_t in_value2 = 6;

		// Write some initial value, pad it, and then write another value
		byte_buffer<32> buffer;
		bit_writer writer(buffer);
		BS_TEST_ASSERT(writer.serialize_bits(in_value1, 3));
		BS_TEST_ASSERT(writer.pad_to_size(31));
		BS_TEST_ASSERT(writer.serialize_bits(in_value2, 5));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 32);

		// Read the values and validate padding
		uint32_t out_value1;
		uint32_t out_value2;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, 3));
		BS_TEST_ASSERT(reader.pad_to_size(31));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, 5));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(out_value2 == in_value2);
	}

	void test_serialize_align()
	{
		// Test align
		uint32_t value = 3;

		// Write a bit offset and the align to byte
		byte_buffer<8> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(value, 2));
		BS_TEST_ASSERT(writer.align());
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 1);

		// Read back the the bit offset and validate the alignment
		uint32_t out_value;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value, 2));
		BS_TEST_ASSERT(reader.get_remaining_bits() > 0);
		BS_TEST_ASSERT(reader.align());

		BS_TEST_ASSERT(out_value == value);
		BS_TEST_ASSERT(reader.get_remaining_bits() == 0);
	}

	BS_ADD_TEST(test_serialize_bytes_small)
	{
		// Test serializing bytes
		uint8_t in_value[2]{ 0xDE, 0x3F }; // The last element is 2^6-1, which just barely fits
		uint8_t in_padding = 27;
		uint32_t num_bits = 2 * 8 - 2;

		// Write some values with a few bits
		byte_buffer<8> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(in_padding, 5));
		BS_TEST_ASSERT(writer.serialize_bytes(in_value, num_bits));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 3);

		// Read the values back and validate
		uint8_t out_value[2];
		uint32_t out_padding;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 5));
		BS_TEST_ASSERT(reader.serialize_bytes(out_value, num_bits));

		BS_TEST_ASSERT(out_padding == in_padding);

		for (int i = 0; i < 2; i++)
			BS_TEST_ASSERT(out_value[i] == in_value[i]);
	}

	BS_ADD_TEST(test_serialize_bytes_medium)
	{
		// Test serializing bytes
		uint8_t in_value[5]{ 0xDE, 0xAD, 0xBE, 0xEE, 0x45 }; // The last element is less than 2^7-1
		uint8_t in_padding = 27;
		uint32_t num_bits = 5 * 8 - 1;

		// Write some values with a few bits
		byte_buffer<8> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(in_padding, 5));
		BS_TEST_ASSERT(writer.serialize_bytes(in_value, num_bits));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 6);

		// Read the values back and validate
		uint8_t out_value[5];
		uint32_t out_padding;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 5));
		BS_TEST_ASSERT(reader.serialize_bytes(out_value, num_bits));

		BS_TEST_ASSERT(out_padding == in_padding);

		for (int i = 0; i < 5; i++)
			BS_TEST_ASSERT(out_value[i] == in_value[i]);
	}

	BS_ADD_TEST(test_serialize_bytes_large)
	{
		// Test serializing bytes
		uint8_t in_value[10]{ 0xDE, 0xAD, 0xBE, 0xEE, 0xEE, 0xEF, 0xFE, 0xAA, 0xC0, 0x1F }; // The last element is 2^5-1, which just barely fits
		uint8_t in_padding = 27;
		uint32_t num_bits = 10 * 8 - 3;

		// Write some values with a few bits
		byte_buffer<16> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(in_padding, 5));
		BS_TEST_ASSERT(writer.serialize_bytes(in_value, num_bits));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 11);

		// Read the values back and validate
		uint8_t out_value[10];
		uint32_t out_padding;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 5));
		BS_TEST_ASSERT(reader.serialize_bytes(out_value, num_bits));

		BS_TEST_ASSERT(out_padding == in_padding);

		for (int i = 0; i < 10; i++)
			BS_TEST_ASSERT(out_value[i] == in_value[i]);
	}

	BS_ADD_TEST(test_serialize_nested_write)
	{
		// Test nested writers
		uint32_t value = 3;
		uint32_t nested_value = 511;

		// Write some initial value with a bit offset
		byte_buffer<8> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(value, 2));
		BS_TEST_ASSERT(writer.serialize_bits(value, 3));

		{
			// Write nested values
            byte_buffer<8> nested_buffer;
            bit_writer nested_writer(nested_buffer);

			BS_TEST_ASSERT(nested_writer.serialize_bits(nested_value, 11));
			BS_TEST_ASSERT(nested_writer.serialize_bits(nested_value, 13));
			BS_TEST_ASSERT(nested_writer.serialize_bits(nested_value, 11));
			uint32_t num_nested_bytes = nested_writer.flush();

			// Copy the nested buffer into the main writer
			BS_TEST_ASSERT(nested_writer.serialize_into(writer));
		}

		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 5);

		// Read the values back, as if it was one big buffer
		uint32_t out_value1;
		uint32_t out_value2;
		uint32_t out_nested_value1;
		uint32_t out_nested_value2;
		uint32_t out_nested_value3;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, 2));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, 3));
		BS_TEST_ASSERT(reader.serialize_bits(out_nested_value1, 11));
		BS_TEST_ASSERT(reader.serialize_bits(out_nested_value2, 13));
		BS_TEST_ASSERT(reader.serialize_bits(out_nested_value3, 11));

		BS_TEST_ASSERT(out_value1 == value);
		BS_TEST_ASSERT(out_value2 == value);
		BS_TEST_ASSERT(out_nested_value1 == nested_value);
		BS_TEST_ASSERT(out_nested_value2 == nested_value);
		BS_TEST_ASSERT(out_nested_value3 == nested_value);
	}

	BS_ADD_TEST(test_serialize_nested_read)
	{
		// Test nested writers
		uint32_t value = 3;
		uint32_t nested_value = 511;

		// Write some initial value with a bit offset
		byte_buffer<8> buffer;
		bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(value, 2));
		BS_TEST_ASSERT(writer.serialize_bits(value, 3));

		{
			// Write nested values
            byte_buffer<8> nested_buffer;
            bit_writer nested_writer(nested_buffer);

			BS_TEST_ASSERT(nested_writer.serialize_bits(nested_value, 11));
			BS_TEST_ASSERT(nested_writer.serialize_bits(nested_value, 13));
			BS_TEST_ASSERT(nested_writer.serialize_bits(nested_value, 11));
			uint32_t num_nested_bytes = nested_writer.flush();

			BS_TEST_ASSERT(num_nested_bytes == 5);

			// Read the values as a byte array
			bit_reader nested_reader(nested_buffer, num_nested_bytes);
			uint8_t nested_bytes[8]{ 0 };
			BS_TEST_ASSERT(nested_reader.serialize_bytes(nested_bytes, nested_writer.get_num_bits_serialized()));

			// Serialize the nested values into the main writer
			BS_TEST_ASSERT(writer.serialize_bytes(nested_bytes, nested_writer.get_num_bits_serialized()));
		}

		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 5);

		// Read the values back, as if it was one big buffer
		uint32_t out_value1;
		uint32_t out_value2;
		uint32_t out_nested_value1;
		uint32_t out_nested_value2;
		uint32_t out_nested_value3;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_value1, 2));
		BS_TEST_ASSERT(reader.serialize_bits(out_value2, 3));
		BS_TEST_ASSERT(reader.serialize_bits(out_nested_value1, 11));
		BS_TEST_ASSERT(reader.serialize_bits(out_nested_value2, 13));
		BS_TEST_ASSERT(reader.serialize_bits(out_nested_value3, 11));

		BS_TEST_ASSERT(out_value1 == value);
		BS_TEST_ASSERT(out_value2 == value);
		BS_TEST_ASSERT(out_nested_value1 == nested_value);
		BS_TEST_ASSERT(out_nested_value2 == nested_value);
		BS_TEST_ASSERT(out_nested_value3 == nested_value);
	}
}