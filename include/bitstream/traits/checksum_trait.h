#pragma once
#include "../utility/assert.h"
#include "../utility/crc.h"
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "../stream/serialize_traits.h"

namespace bitstream
{
	/**
	 * @brief Type for checksums
	 * @tparam Version A unique version number
	*/
	template<uint32_t Version>
	struct checksum;

	/**
	 * @brief A trait used to serialize a checksum of the @p Version and the rest of the buffer as the first 32 bits.
	 * This should be called both first and last when reading and writing to a buffer.
	 * @tparam Version A unique version number
	*/
	template<uint32_t Version>
	struct serialize_traits<checksum<Version>>
	{
		constexpr static uint32_t protocol_version = utility::to_big_endian32_const(Version);
		constexpr static uint32_t protocol_size = sizeof(uint32_t);

		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer) noexcept
		{
			if (writer.get_num_bits_serialized() == 0)
				return writer.pad_to_size(4);
			
			uint32_t num_bits = writer.flush();

			BS_ASSERT(num_bits >= 32U);

			// Get buffer info
			uint8_t* byte_buffer = writer.get_buffer();
			uint32_t num_bytes = writer.get_num_bytes_serialized();

			// Generate checksum of version + data
			uint32_t generated_checksum = utility::crc_uint32(protocol_version, byte_buffer + protocol_size, writer.get_num_bytes_serialized() - protocol_size);

			// Put checksum at beginning
			uint32_t* buffer = reinterpret_cast<uint32_t*>(byte_buffer);
			*buffer = utility::to_big_endian32(generated_checksum);

			return true;
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader) noexcept
		{
			if (reader.get_num_bits_serialized() > 0)
				return true;
			
			BS_ASSERT(reader.can_serialize_bits(32U));

			// Get buffer info
			const uint8_t* byte_buffer = reader.get_buffer();
			uint32_t num_bytes = (reader.get_total_bits() - 1U) / 8U + 1U;

			// Generate checksum to compare against
			uint32_t generated_checksum = utility::crc_uint32(protocol_version, byte_buffer + protocol_size, num_bytes - protocol_size);

			// Read the checksum
			uint32_t given_checksum;
			BS_ASSERT(reader.serialize_bits(given_checksum, 32U));

			// Compare the checksum
			return generated_checksum == given_checksum;
		}
	};
}