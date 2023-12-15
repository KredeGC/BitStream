#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>
#include <bitstream/utility/bits.h>

#include <bitstream/traits/integral_traits.h>
#include <bitstream/traits/quantization_traits.h>
#include <bitstream/traits/string_traits.h>

namespace bitstream::test::deduction
{
    BS_ADD_TEST(test_deduce_integrals)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<4> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        int32_t value = -45; // We can choose any value within the range below
        BS_TEST_ASSERT(writer.serialize(value, -90, 40)); // A lower and upper bound which the value will be quantized between

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

        BS_TEST_ASSERT_OPERATION(num_bits, <= , 32);

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        int32_t out_value; // We don't have to initialize it yet
        BS_TEST_ASSERT(reader.serialize(out_value, -90, 40)); // out_value should now have a value of -45

        BS_TEST_ASSERT_OPERATION(out_value, == , value);
    }

    BS_ADD_TEST(test_deduce_chars)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<32> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        const char* value = "Hello world!";
        BS_TEST_ASSERT(writer.serialize(value, 32U)); // The second argument is the maximum size we expect the string to be

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        char out_value[32]; // Set the size to the max size
        BS_TEST_ASSERT(reader.serialize(out_value, 32U)); // out_value should now contain "Hello world!\0"

        BS_TEST_ASSERT(strcmp(out_value, value) == 0);
    }

    BS_ADD_TEST(test_deduce_strings)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<32> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        std::string value = "Hello world!";
        BS_TEST_ASSERT(writer.serialize(value, 32U)); // The second argument is the maximum size we expect the string to be

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        std::string out_value; // The string will be resized if the output doesn't fit
        BS_TEST_ASSERT(reader.serialize(out_value, 32U)); // out_value should now contain "Hello world!"

        BS_TEST_ASSERT_OPERATION(out_value, == , value);
    }

    BS_ADD_TEST(test_deduce_bounded_range)
    {
        // Create a writer, referencing the buffer and its size
        byte_buffer<4> buffer;
        fixed_bit_writer writer(buffer);

        // Write the value
        bounded_range range(1.0f, 4.0f, 1.0f / 128.0f); // Min, Max, Precision
        float value = 1.2345678f;
        BS_TEST_ASSERT(writer.serialize(range, value));

        // Flush the writer's remaining state into the buffer
        uint32_t num_bits = writer.flush();

        BS_TEST_ASSERT_OPERATION(num_bits, <= , 32);

        // Create a reader by moving and invalidating the writer
        bit_reader reader(buffer, num_bits);

        // Read the value back
        float out_value;
        BS_TEST_ASSERT(reader.serialize(range, out_value)); // out_value should now be a value close to 1.2345678f

        BS_TEST_ASSERT_OPERATION(std::abs(value - out_value), <= , range.get_precision());
        BS_TEST_ASSERT_OPERATION(range.get_bits_required(), < , 32);
    }
}