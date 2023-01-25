#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/smallest_three.h"
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>

namespace bitstream
{
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
}