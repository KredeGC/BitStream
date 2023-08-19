#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/half_precision.h"
#include "../quantization/smallest_three.h"
#include "../utility/assert.h"
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "../stream/serialize_traits.h"

#include <cstdint>

namespace bitstream
{
	/**
	 * @brief A trait used to serialize a single-precision float as half-precision
	*/
	template<>
	struct serialize_traits<half_precision>
	{
		template<typename Stream>
		typename utility::is_writing_t<Stream>
        static serialize(Stream& stream, in<float> value) noexcept
		{
			uint32_t int_value = half_precision::quantize(value);
            
			BS_ASSERT(stream.serialize_bits(int_value, 16));

			return true;
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
        static serialize(Stream& stream, out<float> value) noexcept
		{
			uint32_t int_value;
            
			BS_ASSERT(stream.serialize_bits(int_value, 16));
            
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
		typename utility::is_writing_t<Stream>
		static serialize(Stream& stream, in<bounded_range> range, in<float> value) noexcept
		{
			uint32_t int_value = range.quantize(value);
            
			BS_ASSERT(stream.serialize_bits(int_value, range.get_bits_required()));

			return true;
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& stream, in<bounded_range> range, out<float> value) noexcept
		{
			uint32_t int_value;

			BS_ASSERT(stream.serialize_bits(int_value, range.get_bits_required()));

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
		typename utility::is_writing_t<Stream>
		static serialize(Stream& stream, in<Q> value) noexcept
		{
			quantized_quaternion quantized_quat = smallest_three<Q, BitsPerElement>::quantize(value);

			BS_ASSERT(stream.serialize_bits(quantized_quat.m, 2));
			BS_ASSERT(stream.serialize_bits(quantized_quat.a, BitsPerElement));
			BS_ASSERT(stream.serialize_bits(quantized_quat.b, BitsPerElement));
			BS_ASSERT(stream.serialize_bits(quantized_quat.c, BitsPerElement));

			return true;
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& stream, out<Q> value) noexcept
		{
			quantized_quaternion quantized_quat;

			BS_ASSERT(stream.serialize_bits(quantized_quat.m, 2));
			BS_ASSERT(stream.serialize_bits(quantized_quat.a, BitsPerElement));
			BS_ASSERT(stream.serialize_bits(quantized_quat.b, BitsPerElement));
			BS_ASSERT(stream.serialize_bits(quantized_quat.c, BitsPerElement));

			value = smallest_three<Q, BitsPerElement>::dequantize(quantized_quat);

			return true;
		}
	};
}