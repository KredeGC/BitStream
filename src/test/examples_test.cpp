#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>
#include <bitstream/utility/bits.h>

#include <bitstream/traits/integral_traits.h>
#include <bitstream/traits/quantization_traits.h>
#include <bitstream/traits/string_traits.h>

#include <vector>

namespace bitstream::test::example
{
    // Writing the first 5 bits of an int to the buffer, then reading it back
    BS_ADD_TEST(test_example1)
    {
        // Create a writer, referencing the buffer and its size
        std::vector<uint32_t> buffer;
        growing_bit_writer<std::vector<uint32_t>> writer(buffer);

        // Write the value
        uint32_t value = 27; // We can choose any value below 2^5. Otherwise we need more than 5 bits
        BS_TEST_ASSERT(writer.serialize_bits(value, 5));

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, == , 5);

        // Create a reader, referencing the buffer and bytes written
        bit_reader reader(buffer.data(), num_bits);

        // Read the value back
        uint32_t out_value; // We don't have to initialize it yet
        BS_TEST_ASSERT(reader.serialize_bits(out_value, 5)); // out_value should now have a value of 27
        
		BS_TEST_ASSERT_OPERATION(out_value, == , value);
    }
    
    // Writing a signed int to the buffer, within a range
    BS_ADD_TEST(test_example2)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<4> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        int32_t value = -45; // We can choose any value within the range below
        BS_TEST_ASSERT(writer.serialize<int32_t>(value, -90, 40)); // A lower and upper bound which the value will be quantized between

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, <= , 32);

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        int32_t out_value; // We don't have to initialize it yet
        BS_TEST_ASSERT(reader.serialize<int32_t>(out_value, -90, 40)); // out_value should now have a value of -45
        
		BS_TEST_ASSERT_OPERATION(out_value, == , value);
    }
    
    // Writing a c-style string into the buffer
    BS_ADD_TEST(test_example3)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<32> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        const char* value = "Hello world!";
        BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U)); // The second argument is the maximum size we expect the string to be

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        char out_value[32]; // Set the size to the max size
        BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U)); // out_value should now contain "Hello world!\0"
        
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
    }
    
    // Writing a std::string into the buffer
    BS_ADD_TEST(test_example4)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<32> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        std::string value = "Hello world!";
        BS_TEST_ASSERT(writer.serialize<std::string>(value, 32U)); // The second argument is the maximum size we expect the string to be

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        std::string out_value; // The string will be resized if the output doesn't fit
        BS_TEST_ASSERT(reader.serialize<std::string>(out_value, 32U)); // out_value should now contain "Hello world!"
        
		BS_TEST_ASSERT_OPERATION(out_value, == , value);
    }
    
    // Writing a float into the buffer with a bounded range and precision
    BS_ADD_TEST(test_example5)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<4> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        bounded_range range(1.0f, 4.0f, 1.0f / 128.0f); // Min, Max, Precision
        float value = 1.2345678f;
        BS_TEST_ASSERT(writer.serialize<bounded_range>(range, value));

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, <= , 32);

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        float out_value;
        BS_TEST_ASSERT(reader.serialize<bounded_range>(range, out_value)); // out_value should now be a value close to 1.2345678f

		BS_TEST_ASSERT_OPERATION(std::abs(value - out_value), <= , range.get_precision());
		BS_TEST_ASSERT_OPERATION(range.get_bits_required(), < , 32);
    }
}