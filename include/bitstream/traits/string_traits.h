#pragma once
#include "../utility/assert.h"
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>
#include <string>

namespace bitstream
{
	/**
	 * @brief A trait used to serialize bounded c-style strings
	*/
	template<>
	struct serialize_traits<const char*>
	{
		/**
		 * @brief Writes a c-style string into the @p writer
		 * @param writer The stream to write to
		 * @param value The string to serialize
		 * @param max_size The maximum expected length of the string
		 * @return Success
		*/
		static bool serialize(bit_writer& writer, const char* value, uint32_t max_size) noexcept
		{
			uint32_t length = static_cast<uint32_t>(strlen(value));

			BS_ASSERT(length < max_size);
            
            if (length == 0)
                return true;
            
			uint32_t num_bits = utility::bits_to_represent(max_size);
            
			BS_ASSERT(writer.serialize_bits(length, num_bits));

			return writer.serialize_bytes(reinterpret_cast<const uint8_t*>(value), length * 8);
		}

		/**
		 * @brief Read a c-style string from the @p reader into @p value
		 * @param reader The stream to read from
		 * @param value A pointer to the buffer that should be read into. The size of this buffer should be at least @p max_size
		 * @param max_size The maximum expected length of the string
		 * @return Success
		*/
		static bool serialize(bit_reader& reader, char* value, uint32_t max_size) noexcept
		{
			uint32_t num_bits = utility::bits_to_represent(max_size);

			uint32_t length;
			BS_ASSERT(reader.serialize_bits(length, num_bits));

			BS_ASSERT(length < max_size);

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

	/**
	 * @brief A trait used to serialize any combination of std::basic_string
	 * @tparam T The character type to use
	 * @tparam Traits The trait type for the T type
	 * @tparam Alloc The allocator to use
	*/
	template<typename T, typename Traits, typename Alloc>
	struct serialize_traits<std::basic_string<T, Traits, Alloc>>
	{
		/**
		 * @brief Writes a string into the @p writer
		 * @param writer The stream to write to
		 * @param value The string to serialize
		 * @param max_size The maximum expected length of the string
		 * @return Success
		*/
		static bool serialize(bit_writer& writer, const std::basic_string<T, Traits, Alloc>& value, uint32_t max_size) noexcept
		{
			uint32_t length = static_cast<uint32_t>(value.size());

			BS_ASSERT(length < max_size);

			uint32_t num_bits = utility::bits_to_represent(max_size);

			BS_ASSERT(writer.serialize_bits(length, num_bits));

			if (length == 0)
				return true;

			return writer.serialize_bytes(reinterpret_cast<const uint8_t*>(value.c_str()), length * sizeof(T) * 8);
		}

		/**
		 * @brief Reads a string from the @p reader into @p value
		 * @param reader The stream to read from
		 * @param value The string to read into. It will be resized if the read string won't fit
		 * @param max_size The maximum expected length of the string
		 * @return Success
		*/
		static bool serialize(bit_reader& reader, std::basic_string<T, Traits, Alloc>& value, uint32_t max_size) noexcept
		{
			uint32_t num_bits = utility::bits_to_represent(max_size);

			uint32_t length;
			BS_ASSERT(reader.serialize_bits(length, num_bits));

			BS_ASSERT(length < max_size);

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