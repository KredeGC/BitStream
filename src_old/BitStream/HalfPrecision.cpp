#include "HalfPrecision.h"

/*
 *  Copyright (c) 2018 Stanislav Denisov
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
	uint16_t HalfPrecision::quantize(float value)
	{
		Values values
		{
			value
		};

		return quantize(values.i);
	}

	uint16_t HalfPrecision::quantize(int32_t value)
	{
		int32_t s = (value >> 16) & 0x00008000;
		int32_t e = ((value >> 23) & 0X000000FF) - (127 - 15);
		int32_t m = value & 0X007FFFFF;

		if (e <= 0) {
			if (e < -10)
				return (uint16_t)s;

			m |= 0x00800000;

			int32_t t = 14 - e;
			int32_t a = (1 << (t - 1)) - 1;
			int32_t b = (m >> t) & 1;

			m = (m + a + b) >> t;

			return (uint16_t)(s | m);
		}

		if (e == 0XFF - (127 - 15)) {
			if (m == 0)
				return (uint16_t)(s | 0X7C00);

			m >>= 13;

			return (uint16_t)(s | 0X7C00 | m | ((m == 0) ? 1 : 0));
		}

		m = m + 0X00000FFF + ((m >> 13) & 1);

		if ((m & 0x00800000) != 0) {
			m = 0;
			e++;
		}

		if (e > 30)
			return (uint16_t)(s | 0X7C00);

		return (uint16_t)(s | (e << 10) | (m >> 13));
	}

	float HalfPrecision::dequantize(uint16_t value)
	{
		uint32_t result;
		uint32_t mantissa = (uint32_t)(value & 1023);
		uint32_t exponent = 0XFFFFFFF2;

		if ((value & -33792) == 0) {
			if (mantissa != 0) {
				while ((mantissa & 1024) == 0) {
					exponent--;
					mantissa <<= 1;
				}

				mantissa &= 0XFFFFFBFF;
				result = (((uint32_t)value & 0x8000) << 16) | ((exponent + 127) << 23) | (mantissa << 13);
			}
			else
			{
				result = (uint32_t)((value & 0x8000) << 16);
			}
		}
		else
		{
			result = ((((uint32_t)value & 0x8000) << 16) | ((((((uint32_t)value >> 10) & 0X1F) - 15) + 127) << 23)) | (mantissa << 13);
		}

		Values values;
		values.i = result;

		return values.f;
	}
}