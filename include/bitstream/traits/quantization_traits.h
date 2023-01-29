#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/half_precision.h"
#include "../quantization/smallest_three.h"

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
		static bool serialize(Stream& stream, float& value) noexcept
		{
			union float_int
			{
				float f;
				uint32_t i;
			};

			float_int tmp;
            
            if constexpr (Stream::writing)
                tmp.f = value;

			BS_ASSERT(stream.serialize_bits(tmp.i, 32));
            
            if constexpr (Stream::reading)
			    value = tmp.f;

			return true;
		}
	};

	template<>
	struct serialize_traits<half_precision>
	{
        template<typename Stream>
        static bool serialize(Stream& stream, float& value) noexcept
		{
			uint32_t int_value;
            if constexpr (Stream::writing)
                int_value = half_precision::quantize(value);
            
			BS_ASSERT(stream.serialize_bits(int_value, 16));
            
            if constexpr (Stream::reading)
			    value = half_precision::dequantize(int_value);

			return true;
		}
	};

	template<>
	struct serialize_traits<bounded_range>
	{
        template<typename Stream>
		static bool serialize(Stream& stream, const bounded_range& range, float& value) noexcept
		{
			uint32_t int_value;
            if constexpr (Stream::writing)
                int_value = range.quantize(value);
            
			BS_ASSERT(stream.serialize_bits(int_value, range.get_bits_required()));

            if constexpr (Stream::reading)
			    value = range.dequantize(int_value);

			return true;
		}
	};

	template<typename Q, size_t BitsPerElement>
	struct serialize_traits<smallest_three<Q, BitsPerElement>>
	{
		static bool serialize(bit_writer& writer, const Q& value) noexcept
		{
			auto quantized_quat = smallest_three<Q, BitsPerElement>::quantize(value);

			BS_ASSERT(writer.serialize_bits(quantized_quat.m, 2));

			BS_ASSERT(writer.serialize_bits(quantized_quat.a, BitsPerElement));

			BS_ASSERT(writer.serialize_bits(quantized_quat.b, BitsPerElement));

			return writer.serialize_bits(quantized_quat.c, BitsPerElement);
		}

		static bool serialize(bit_reader& reader, Q& value) noexcept
		{
			quantized_quaternion quantized_quat;

			BS_ASSERT(reader.serialize_bits(quantized_quat.m, 2));

			BS_ASSERT(reader.serialize_bits(quantized_quat.a, BitsPerElement));

			BS_ASSERT(reader.serialize_bits(quantized_quat.b, BitsPerElement));

			BS_ASSERT(reader.serialize_bits(quantized_quat.c, BitsPerElement));

			value = smallest_three<Q, BitsPerElement>::dequantize(quantized_quat);

			return true;
		}
	};
}