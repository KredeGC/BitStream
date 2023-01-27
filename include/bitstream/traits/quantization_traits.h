#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/half_precision.h"
#include "../quantization/smallest_three.h"
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>

namespace bitstream
{
	template<>
	struct serialize_traits<float>
	{
        template<typename Stream>
		static bool serialize(Stream& stream, float& value)
		{
			union float_int
			{
				float f;
				uint32_t i;
			};

			float_int tmp;
            
            if constexpr (Stream::writing)
                tmp.f = value;

			if (!stream.serialize_bits(tmp.i, 32))
				return false;
            
            if constexpr (Stream::reading)
			    value = tmp.f;

			return true;
		}
	};

	template<>
	struct serialize_traits<half_precision>
	{
        template<typename Stream>
        static bool serialize(Stream& stream, float& value)
		{
			uint32_t int_value;
            if constexpr (Stream::writing)
                int_value = half_precision::quantize(value);
            
			if (!stream.serialize_bits(int_value, 16))
				return false;
            
            if constexpr (Stream::reading)
			    value = half_precision::dequantize(int_value);

			return true;
		}
	};

	template<>
	struct serialize_traits<bounded_range>
	{
        template<typename Stream>
		static bool serialize(Stream& stream, const bounded_range& range, float& value)
		{
			uint32_t int_value;
            if constexpr (Stream::writing)
                int_value = range.quantize(value);
            
			if (!stream.serialize_bits(int_value, range.get_bits_required()))
				return false;

            if constexpr (Stream::reading)
			    value = range.dequantize(int_value);

			return true;
		}
	};

	template<typename Q, size_t BitsPerElement>
	struct serialize_traits<smallest_three<Q, BitsPerElement>>
	{
		static bool serialize(bit_writer& writer, const Q& value)
		{
			auto quantized_quat = smallest_three<Q, BitsPerElement>::quantize(value);

			if (!writer.serialize_bits(quantized_quat.m, 2))
				return false;

			if (!writer.serialize_bits(quantized_quat.a, BitsPerElement))
				return false;

			if (!writer.serialize_bits(quantized_quat.b, BitsPerElement))
				return false;

			return writer.serialize_bits(quantized_quat.c, BitsPerElement);
		}

		static bool serialize(bit_reader& reader, Q& value)
		{
			quantized_quaternion quantized_quat;

			if (!reader.serialize_bits(quantized_quat.m, 2))
				return false;

			if (!reader.serialize_bits(quantized_quat.a, BitsPerElement))
				return false;

			if (!reader.serialize_bits(quantized_quat.b, BitsPerElement))
				return false;

			if (!reader.serialize_bits(quantized_quat.c, BitsPerElement))
				return false;

			value = smallest_three<Q, BitsPerElement>::dequantize(quantized_quat);

			return true;
		}
	};
}