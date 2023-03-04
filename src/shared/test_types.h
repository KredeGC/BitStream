#pragma once

#include <bitstream/utility/assert.h>

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
            BS_TEST_ASSERT(index >= 0 && index < 4);
            
            return values[index];
		}
	};
}