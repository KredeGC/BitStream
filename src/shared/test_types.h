#pragma once

#include <bitstream/stream/serialize_traits.h>

#include <bitstream/utility/assert.h>
#include <bitstream/utility/parameter.h>

#include <cstddef>

namespace bitstream::test
{
    struct quaternion
	{
		// smallest_three supports any combination of w, x, y and z, as long as it's consistent
		float values[4];

		// The constructor order must be the same as the operator[]
		float operator[](size_t index) const
		{
            BS_TEST_ASSERT(index < 4);
            
            return values[index];
		}
	};

    struct custom_type
    {
        bool enabled = true;
        int count = 42;
    };
    
    enum class test_enum
    {
        FirstValue = 3,
        SecondValue = 1,
        ThirdValue
    };
}

namespace bitstream
{
    template<>
    struct serialize_traits<bitstream::test::custom_type>
    {
        template<typename Stream>
        static bool serialize(Stream& stream, inout<Stream, bitstream::test::custom_type> value) noexcept
        {
            if (!stream.template serialize<bool>(value.enabled))
                return false;

            return stream.template serialize<int>(value.count);
        }
    };
}