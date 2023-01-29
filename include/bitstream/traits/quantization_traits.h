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
	/**
	 * @brief A trait used to serialize a float as-is, without any bound checking or quantization
	*/
	template<>
	struct serialize_traits<float>
	{
        /**
		 * @brief Serializes a whole float into or out of the @p stream
         * @tparam Stream The type of stream to use. Usually bit_writer or bit_reader
		 * @param stream The stream to serialize to
		 * @param value The string to serialize
		 * @return Success
        */
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

	/**
	 * @brief A trait used to serialize a single-precision float as half-precision
	*/
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

	/**
	 * @brief A trait used to quantize and serialize a float to be within a given range and precision
	*/
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

	/**
	 * @brief A trait used to quantize and serialize quaternions using the smallest-three algorithm
	*/
	template<typename Q, size_t BitsPerElement>
	struct serialize_traits<smallest_three<Q, BitsPerElement>>
	{
		template<typename Stream>
		static bool serialize(Stream& reader, Q& value) noexcept
		{
			quantized_quaternion quantized_quat;

			if constexpr (Stream::writing)
				quantized_quat = smallest_three<Q, BitsPerElement>::quantize(value);

			BS_ASSERT(reader.serialize_bits(quantized_quat.m, 2));

			BS_ASSERT(reader.serialize_bits(quantized_quat.a, BitsPerElement));

			BS_ASSERT(reader.serialize_bits(quantized_quat.b, BitsPerElement));

			BS_ASSERT(reader.serialize_bits(quantized_quat.c, BitsPerElement));

			if constexpr (Stream::reading)
				value = smallest_three<Q, BitsPerElement>::dequantize(quantized_quat);

			return true;
		}
	};
}