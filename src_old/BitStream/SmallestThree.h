#pragma once

#include <cstdint>
#include <limits>
#include <cmath>

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

namespace BSNet::Quantization
{
	struct QuantizedQuaternion
	{
		uint32_t m;
		uint32_t a;
		uint32_t b;
		uint32_t c;

		QuantizedQuaternion() = default;

		QuantizedQuaternion(uint32_t m, uint32_t a, uint32_t b, uint32_t c)
			: m(m), a(a), b(b), c(c) {}
	};

	class SmallestThree
	{
	private:
		static constexpr float SMALLEST_THREE_UNPACK = 0.70710678118654752440084436210485f + 0.0000001f;
		static constexpr float SMALLEST_THREE_PACK = 1.0f / SMALLEST_THREE_UNPACK;

	public:
		template<typename T>
		inline static QuantizedQuaternion quantize(const T& quaternion, uint32_t bits_per_element = 12)
		{
			float half_range = (float)(1 << (bits_per_element - 1));
			float packer = SMALLEST_THREE_PACK * half_range;
			float max_value = std::numeric_limits<float>::lowest();
			bool sign_minus = false;
			uint32_t m = 0;
			uint32_t a = 0;
			uint32_t b = 0;
			uint32_t c = 0;

			for (uint32_t i = 0; i < 4; i++) {
				float element = 0.0f;

				switch (i)
				{
				case 0:
					element = quaternion.x;
					break;
				case 1:
					element = quaternion.y;
					break;
				case 2:
					element = quaternion.z;
					break;
				case 3:
					element = quaternion.w;
					break;
				}

				float abs = std::abs(element);

				if (abs > max_value)
				{
					sign_minus = (element < 0.0f);
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
				af = quaternion.y;
				bf = quaternion.z;
				cf = quaternion.w;
				break;
			case 1:
				af = quaternion.x;
				bf = quaternion.z;
				cf = quaternion.w;
				break;
			case 2:
				af = quaternion.x;
				bf = quaternion.y;
				cf = quaternion.w;
				break;
			default:
				af = quaternion.x;
				bf = quaternion.y;
				cf = quaternion.z;
				break;
			}

			if (sign_minus)
			{
				a = (uint32_t)((-af * packer) + half_range);
				b = (uint32_t)((-bf * packer) + half_range);
				c = (uint32_t)((-cf * packer) + half_range);
			}
			else
			{
				a = (uint32_t)((af * packer) + half_range);
				b = (uint32_t)((bf * packer) + half_range);
				c = (uint32_t)((cf * packer) + half_range);
			}

			return { m, a, b, c };
		}

		template<typename T>
		inline static T dequantize(const QuantizedQuaternion& data, uint32_t bitsPerElement = 12)
		{
			int32_t half_range = (1 << (bitsPerElement - 1));
			float unpacker = SMALLEST_THREE_UNPACK * (1.0f / half_range);
			uint32_t m = data.m;
			int32_t ai = (int32_t)data.a;
			int32_t bi = (int32_t)data.b;
			int32_t ci = (int32_t)data.c;

			ai -= half_range;
			bi -= half_range;
			ci -= half_range;

			float a = ai * unpacker;
			float b = bi * unpacker;
			float c = ci * unpacker;

			float d = (float)std::sqrt(1.0f - ((a * a) + (b * b) + (c * c)));

			switch (m)
			{
			case 0:
				return T(c, d, a, b);
			case 1:
				return T(c, a, d, b);
			case 2:
				return T(c, a, b, d);
			default:
				return T(d, a, b, c);
			}
		}
	};
}