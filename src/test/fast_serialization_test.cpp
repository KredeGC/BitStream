#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>
#include <bitstream/utility/bits.h>

#include <bitstream/traits/integral_traits.h>
#include <bitstream/traits/quantization_traits.h>
#include <bitstream/traits/string_traits.h>

#include <chrono>
#include <iostream>

namespace bitstream::test::performance
{
    template<typename F>
    void profile_time(F&& func)
    {
        auto start = std::chrono::steady_clock::now();

        bool status = func();

        auto end = std::chrono::steady_clock::now();

        auto start_time = std::chrono::time_point_cast<std::chrono::nanoseconds>(start).time_since_epoch();
        auto end_time = std::chrono::time_point_cast<std::chrono::nanoseconds>(end).time_since_epoch();

        auto time = (end_time - start_time).count();

        std::cout << "  ->time spent: " << time << "ns\n";

        BS_TEST_ASSERT(status);
    }

    // Byte copying will have the wrong endianness for the type
    // But at least it is fast
    BS_ADD_TEST(test_aligned_bytecopy_performance)
    {
        byte_buffer<4096> buffer;
        fixed_bit_writer writer(buffer);

        uint32_t numbers[1024];
        for (uint32_t i = 0U; i < 1024U; i++)
            numbers[i] = 24U;

        // Should just use straight memcpy
        profile_time([&]
        {
            BS_ASSERT(writer.serialize_bytes(reinterpret_cast<uint8_t*>(numbers), 1024U * 8U * sizeof(uint32_t)));

            writer.flush();

            return true;
        });
    }

    BS_ADD_TEST(test_misaligned_bytecopy_performance)
    {
        byte_buffer<4100> buffer;
        fixed_bit_writer writer(buffer);

        uint32_t numbers[1024];
        for (uint32_t i = 0U; i < 1024U; i++)
            numbers[i] = 24U;

        // Force misalignment
        BS_TEST_ASSERT(writer.serialize_bits(0, 1));

        // Has to use serialize_bits individually
        profile_time([&]
        {
            BS_ASSERT(writer.serialize_bytes(reinterpret_cast<uint8_t*>(numbers), 1024U * 8U * sizeof(uint32_t)));

            writer.flush();

            return true;
        });
    }

    BS_ADD_TEST(test_aligned_performance)
    {
        byte_buffer<4096> buffer;
        fixed_bit_writer writer(buffer);

        // Should use the fast-path of serialize_bits
        profile_time([&]
        {
            for (uint32_t i = 0U; i < 1024U; i++)
            {
                BS_ASSERT(writer.serialize_bits(24U, 32U));
            }

            writer.flush();

            return true;
        });
    }

    BS_ADD_TEST(test_misaligned_performance)
    {
        byte_buffer<4096> buffer;
        fixed_bit_writer writer(buffer);

        // Has to use the slow path of serialize_bits
        profile_time([&]
        {
            for (uint32_t i = 0U; i < 1024U; i++)
            {
                BS_ASSERT(writer.serialize_bits(24U, 31U));
            }

            writer.flush();

            return true;
        });
    }
}