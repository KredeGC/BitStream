#pragma once

#include <cstdint>

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

namespace bitstream::quantization
{
	class bounded_range
	{
	public:
		bounded_range() = default;

		bounded_range(float min, float max, float precision) :
		    m_Min(min),
            m_Max(max),
            m_Precision(precision)
        {
            m_BitsRequired = log2(static_cast<uint32_t>((m_Max - m_Min) * (1.0f / precision) + 0.5f)) + 1;
            m_Mask = static_cast<uint32_t>((1L << m_BitsRequired) - 1);
        }
        
        inline float get_min() const { return m_Min; }
        inline float get_max() const { return m_Max; }
        inline float get_precision() const { return m_Precision; }
        inline uint32_t get_bits_required() const { return m_BitsRequired; }

		inline uint32_t quantize(float value) const
        {
            if (value < m_Min)
                value = m_Min;
            else if (value > m_Max)
                value = m_Max;

            return static_cast<uint32_t>(static_cast<float>((value - m_Min) * (1.0f / m_Precision)) + 0.5f) & m_Mask;
        }
        
		inline float dequantize(uint32_t data) const
        {
            float adjusted = (static_cast<float>(data) * m_Precision) + m_Min;

            if (adjusted < m_Min)
                adjusted = m_Min;
            else if (adjusted > m_Max)
                adjusted = m_Max;

            return adjusted;
        }

	private:
		inline static constexpr uint32_t log2(uint32_t value)
        {
            value |= value >> 1;
            value |= value >> 2;
            value |= value >> 4;
            value |= value >> 8;
            value |= value >> 16;

            return DE_BRUIJN[(value * 0x07C4ACDDU) >> 27];
        }
        
	private:
        float m_Min;
		float m_Max;
		float m_Precision;

		uint32_t m_BitsRequired;
		uint32_t m_Mask;
        
        inline static constexpr uint32_t DE_BRUIJN[32]
        {
             0,  9,  1, 10, 13, 21,  2, 29,
            11, 14, 16, 18, 22, 25,  3, 30,
             8, 12, 20, 28, 15, 17, 24,  7,
            19, 27, 23,  6, 26,  5,  4, 31
        };
	};
}