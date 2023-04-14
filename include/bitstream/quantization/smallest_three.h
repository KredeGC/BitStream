#pragma once

/*
 *  Copyright (c) 2020 Stanislav Denisov, Maxim Munning, Davin Carten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <cstdint>
#include <cmath>

namespace bitstream
{
	/**
	 * @brief A quantized representation of a quaternion
	*/
	struct quantized_quaternion
	{
		uint32_t m;
		uint32_t a;
		uint32_t b;
		uint32_t c;

		constexpr quantized_quaternion() noexcept :
			m(0),
			a(0),
			b(0),
			c(0) {}

		constexpr quantized_quaternion(uint32_t m, uint32_t a, uint32_t b, uint32_t c) noexcept :
			m(m), a(a), b(b), c(c) {}
	};

	/**
	 * @brief Class for quantizing a user-specified quaternion into fewer bits using the smallest-three algorithm
	 * @tparam T The quaternion-type to quantize
	*/
	template<typename T, size_t BitsPerElement = 12>
	class smallest_three
	{
	private:
		static constexpr float SMALLEST_THREE_UNPACK = 0.70710678118654752440084436210485f + 0.0000001f;
		static constexpr float SMALLEST_THREE_PACK = 1.0f / SMALLEST_THREE_UNPACK;

	public:
		inline static quantized_quaternion quantize(const T& quaternion) noexcept
		{
			constexpr float half_range = static_cast<float>(1 << (BitsPerElement - 1));
			constexpr float packer = SMALLEST_THREE_PACK * half_range;
            
			float max_value = -1.0f;
			bool sign_minus = false;
			uint32_t m = 0;
			uint32_t a = 0;
			uint32_t b = 0;
			uint32_t c = 0;

			for (uint32_t i = 0; i < 4; i++)
            {
				float element = quaternion[i];

				float abs = element > 0.0f ? element : -element;

				if (abs > max_value)
				{
					sign_minus = element < 0.0f;
					m = i;
					max_value = abs;
				}
			}

			float af = 0.0f;
			float bf = 0.0f;
			float cf = 0.0f;

			switch (m)
			{
			case 0:
				af = quaternion[1];
				bf = quaternion[2];
				cf = quaternion[3];
				break;
			case 1:
				af = quaternion[0];
				bf = quaternion[2];
				cf = quaternion[3];
				break;
			case 2:
				af = quaternion[0];
				bf = quaternion[1];
				cf = quaternion[3];
				break;
			default: // case 3
				af = quaternion[0];
				bf = quaternion[1];
				cf = quaternion[2];
				break;
			}

			if (sign_minus)
			{
				a = static_cast<uint32_t>((-af * packer) + half_range);
				b = static_cast<uint32_t>((-bf * packer) + half_range);
				c = static_cast<uint32_t>((-cf * packer) + half_range);
			}
			else
			{
				a = static_cast<uint32_t>((af * packer) + half_range);
				b = static_cast<uint32_t>((bf * packer) + half_range);
				c = static_cast<uint32_t>((cf * packer) + half_range);
			}

			return { m, a, b, c };
		}

		inline static T dequantize(const quantized_quaternion& data) noexcept
		{
			constexpr uint32_t half_range = (1 << (BitsPerElement - 1));
			constexpr float unpacker = SMALLEST_THREE_UNPACK * (1.0f / half_range);
            
			float a = data.a * unpacker - half_range * unpacker;
			float b = data.b * unpacker - half_range * unpacker;
			float c = data.c * unpacker - half_range * unpacker;

			float d = std::sqrt(1.0f - ((a * a) + (b * b) + (c * c)));

			switch (data.m)
			{
			case 0:
				return T{ d, a, b, c };
			case 1:
				return T{ a, d, b, c };
			case 2:
				return T{ a, b, d, c };
			default: // case 3
				return T{ a, b, c, d };
			}
		}
	};
}