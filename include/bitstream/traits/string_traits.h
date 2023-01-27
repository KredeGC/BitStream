#pragma once
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>
#include <string>
#include <type_traits>

namespace bitstream
{
	template<>
	struct serialize_traits<const char*>
	{
		static bool serialize(bit_writer& writer, const char* value, uint32_t max_size) noexcept
		{
			uint32_t length = static_cast<uint32_t>(strlen(value));
			if (length >= max_size)
				return false;
            
            if (length == 0)
                return true;
            
			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));
            
			BS_ASSERT(writer.serialize_bits(length, num_bits));

			return writer.serialize_bytes(reinterpret_cast<const uint8_t*>(value), length * 8);
		}

		static bool serialize(bit_reader& reader, char* value, uint32_t max_size) noexcept
		{
			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			uint32_t length;
			BS_ASSERT(reader.serialize_bits(length, num_bits));

			if (length >= max_size)
				return false;

			if (length == 0)
			{
				value[0] = '\0';
				return true;
			}

			BS_ASSERT(reader.serialize_bytes(reinterpret_cast<uint8_t*>(value), length * 8));

			value[length] = '\0';

			return true;
		}
	};

	template<typename T, typename Traits, typename Alloc>
	struct serialize_traits<std::basic_string<T, Traits, Alloc>>
	{
		static bool serialize(bit_writer& writer, const std::basic_string<T, Traits, Alloc>& value, uint32_t max_size) noexcept
		{
			uint32_t length = static_cast<uint32_t>(value.size());
			if (length >= max_size)
				return false;

			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			BS_ASSERT(writer.serialize_bits(length, num_bits));

			if (length == 0)
				return true;

			return writer.serialize_bytes(reinterpret_cast<const uint8_t*>(value.c_str()), length * sizeof(T) * 8);
		}

		static bool serialize(bit_reader& reader, std::basic_string<T, Traits, Alloc>& value, uint32_t max_size) noexcept
		{
			int num_bits = static_cast<int>(utility::bits_to_represent(max_size));

			uint32_t length;
			BS_ASSERT(reader.serialize_bits(length, num_bits));

			if (length >= max_size)
				return false;

			if (length == 0)
			{
				value.clear();
				return true;
			}

            value.resize(length);

			BS_ASSERT(reader.serialize_bytes(reinterpret_cast<uint8_t*>(value.data()), length * sizeof(T) * 8));

			return true;
		}
	};
}