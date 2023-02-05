#pragma once
#include "../utility/assert.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

namespace bitstream
{
	/**
	 * @brief A trait used to serialize a boolean as a single bit
	*/
	template<>
	struct serialize_traits<bool>
	{
		static bool serialize(bit_writer& writer, bool value) noexcept
		{
			uint32_t unsigned_value = value;

			return writer.serialize_bits(unsigned_value, 1U);
		}

		static bool serialize(bit_reader& reader, bool& value) noexcept
		{
			uint32_t unsigned_value;

			BS_ASSERT(reader.serialize_bits(unsigned_value, 1U));

			value = unsigned_value;

			return true;
		}
	};
}