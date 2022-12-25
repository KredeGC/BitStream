#include "bspch.h"
#include "Core/Core.h"
#include "BoundedRange.h"

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
	static constexpr uint32_t DE_BRUIJN[32]
	{
		0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
	};

	BoundedRange::BoundedRange(float min_value, float max_value, float precision)
		: min_value(min_value), max_value(max_value), precision(precision)
	{
		BS_ASSERT(min_value < max_value);

		bits_required = log2((uint32_t)((max_value - min_value) * (1.0f / precision) + 0.5f)) + 1;
		mask = (uint32_t)((1L << bits_required) - 1);
	}

	uint32_t BoundedRange::quantize(float value) const
	{
		if (value < min_value)
			value = min_value;
		else if (value > max_value)
			value = max_value;

		return (uint32_t)((float)((value - min_value) * (1.0f / precision)) + 0.5f) & mask;
	}

	float BoundedRange::dequantize(uint32_t data) const
	{
		float adjusted = ((float)data * precision) + min_value;

		if (adjusted < min_value)
			adjusted = min_value;
		else if (adjusted > max_value)
			adjusted = max_value;

		return adjusted;
	}

	uint32_t BoundedRange::log2(uint32_t value) {
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;

		return DE_BRUIJN[(value * 0x07C4ACDDU) >> 27];
	}
}