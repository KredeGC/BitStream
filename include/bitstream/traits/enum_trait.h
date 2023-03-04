#pragma once
#include "../utility/assert.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include "../traits/integral_traits.h"

namespace bitstream
{
	/**
	 * @brief A trait used to serialize an enum type
	*/
	template<typename T>
	struct serialize_traits<T, typename std::enable_if_t<std::is_enum_v<T>>>
	{
        using value_type = std::underlying_type_t<T>;
        
		static bool serialize(bit_writer& writer, T value, value_type min = 0, value_type max = (std::numeric_limits<value_type>::max)()) noexcept
		{
			value_type unsigned_value = static_cast<value_type>(value);

			return writer.serialize<value_type>(unsigned_value, min, max);
		}

		static bool serialize(bit_reader& reader, T& value, value_type min = 0, value_type max = (std::numeric_limits<value_type>::max)()) noexcept
		{
			value_type unsigned_value;

			BS_ASSERT(reader.serialize<value_type>(unsigned_value, min, max));

			value = static_cast<T>(unsigned_value);

			return true;
		}
	};
}