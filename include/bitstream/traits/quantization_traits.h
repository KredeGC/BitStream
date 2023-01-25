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
	struct half_float;

	template<>
	struct serialize_traits<float>
	{
		static bool serialize(bit_writer& writer, const float& value)
		{
			if (!writer.can_write_bits(32))
				return false;

			union float_int
			{
				float f;
				uint32_t i;
			};

			float_int tmp;
			tmp.f = value;

			return writer.serialize_bits(tmp.i, 32);
		}

		static bool deserialize(bit_reader& reader, float& value)
		{
			if (!reader.can_read_bits(32))
				return false;

			union float_int
			{
				float f;
				uint32_t i;
			};

			float_int tmp;

			if (!reader.serialize_bits(tmp.i, 32))
				return false;

			value = tmp.f;

			return true;
		}
	};

	template<>
	struct serialize_traits<half_float>
	{
		static bool serialize(bit_writer& writer, const float& value)
		{
			if (!writer.can_write_bits(16))
				return false;

			uint32_t int_value = half_precision::quantize(value);

			return writer.serialize_bits(int_value, 16);
		}

		static bool deserialize(bit_reader& reader, float& value)
		{
			if (!reader.can_read_bits(16))
				return false;

			uint32_t int_value;
			if (!reader.serialize_bits(int_value, 16))
				return false;

			value = half_precision::dequantize(int_value);

			return true;
		}
	};

	template<>
	struct serialize_traits<bounded_range>
	{
		static bool serialize(bit_writer& writer, const bounded_range& range, const float& value)
		{
			if (!writer.can_write_bits(range.get_bits_required()))
				return false;

			uint32_t int_value = range.quantize(value);

			return writer.serialize_bits(int_value, range.get_bits_required());
		}

		static bool deserialize(bit_reader& reader, const bounded_range& range, float& value)
		{
			if (!reader.can_read_bits(range.get_bits_required()))
				return false;

			uint32_t int_value;
			if (!reader.serialize_bits(int_value, range.get_bits_required()))
				return false;

			value = range.dequantize(int_value);

			return true;
		}
	};

	template<typename Q, size_t BitsPerElement>
	struct serialize_traits<smallest_three<Q, BitsPerElement>>
	{
		static bool serialize(bit_writer& writer, const Q& value)
		{
			if (!writer.can_write_bits(2 + 3 * BitsPerElement))
				return false;

			auto quantized_quat = smallest_three<Q, BitsPerElement>::quantize(value);

			if (!writer.serialize_bits(quantized_quat.m, 2))
				return false;

			if (!writer.serialize_bits(quantized_quat.a, BitsPerElement))
				return false;

			if (!writer.serialize_bits(quantized_quat.b, BitsPerElement))
				return false;

			return writer.serialize_bits(quantized_quat.c, BitsPerElement);
		}

		static bool deserialize(bit_reader& reader, Q& value)
		{
			if (!reader.can_read_bits(2 + 3 * BitsPerElement))
				return false;

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