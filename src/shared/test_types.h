#pragma once

#include <cstddef>

namespace bitstream::test
{
	struct quaternion
	{
		// smallest_three supports any combination of w, x, y and z, as long as it's consistent
		float w;
		float x;
		float y;
		float z;

		quaternion() = default;

		// The constructor order must be the same as the operator[]
		quaternion(float w, float x, float y, float z)
			: w(w), x(x), y(y), z(z) {}

		// smallest_three uses this operator
		float operator[](size_t index) const
		{
			return reinterpret_cast<const float*>(this)[index];
		}
	};
}