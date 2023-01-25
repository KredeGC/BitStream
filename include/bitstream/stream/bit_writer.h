#pragma once
#include "../utility/assert.h"
#include "../utility/crc.h"
#include "../utility/endian.h"

#include "serialize_traits.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace bitstream
{
	class bit_writer
	{
	public:
		static constexpr bool writing = true;
		static constexpr bool reading = false;

		bit_writer() :
			m_Buffer(nullptr),
			m_NumBitsWritten(0),
			m_TotalBits(0),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		bit_writer(void* bytes, uint32_t num_bytes) :
			m_Buffer(static_cast<uint32_t*>(bytes)),
			m_NumBitsWritten(0),
			m_TotalBits(num_bytes * 8),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0)
		{
			BS_ASSERT(num_bytes % 4 == 0);
		}

		bit_writer(const bit_writer&) = delete;
		bit_writer(bit_writer&&) = delete;

		uint32_t get_num_bits_written() { return m_NumBitsWritten; }

		bool can_write_bits(uint32_t num_bits) { return m_NumBitsWritten + num_bits <= m_TotalBits; }

		uint32_t get_remaining_bits() { return m_TotalBits - m_NumBitsWritten; }

		uint32_t flush()
		{
			if (m_ScratchBits > 0)
			{
				uint32_t* ptr = m_Buffer + m_WordIndex;
				uint32_t ptr_value = static_cast<uint32_t>(m_Scratch >> 32);
				*ptr = utility::endian_swap_32(ptr_value);

				m_Scratch = 0;
				m_ScratchBits = 0;
				m_WordIndex++;
			}

			return (m_NumBitsWritten - 1) / 8 + 1;
		}

		void prepend_checksum()
		{
			// Advance the reader by the size of the checksum (32 bits / 1 word)
			m_WordIndex++;
			m_NumBitsWritten += 32;
		}

		uint32_t serialize_checksum(uint32_t protocol_version)
		{
			uint32_t num_bytes = flush();

			// Copy protocol version to buffer
			std::memcpy(m_Buffer, &protocol_version, sizeof(uint32_t));

			// Generate checksum of version + data
			uint32_t checksum = utility::crc_uint32(reinterpret_cast<uint8_t*>(m_Buffer), num_bytes);

			// Put checksum at beginning
			std::memcpy(m_Buffer, &checksum, sizeof(uint32_t));

			return num_bytes;
		}

		bool pad_to_size(uint32_t size)
		{
			BS_ASSERT(size * 8 <= m_TotalBits);

			flush();

			if (size * 8 < m_NumBitsWritten)
				return false;

			// Set to the padding to 0
			std::memset(m_Buffer + m_WordIndex, 0, size - m_WordIndex * 4);

			m_NumBitsWritten = size * 8U;

			m_Scratch = 0ULL;
			m_ScratchBits = (size % 4U) * 8U;
			m_WordIndex = size / 4U;

			return true;
		}

		bool align()
		{
			uint32_t remainder = m_ScratchBits % 8;
			if (remainder != 0)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, 8 - remainder);

				return status && m_NumBitsWritten % 8 == 0;
			}
			return true;
		}

		bool serialize_bits(const uint32_t& value, uint32_t num_bits)
		{
			BS_ASSERT(num_bits > 0 && num_bits <= 32);

			BS_ASSERT(can_write_bits(num_bits));

			uint32_t offset = 64 - num_bits - m_ScratchBits;
			uint64_t ls_value = static_cast<uint64_t>(value) << offset;

			m_Scratch |= ls_value;
			m_ScratchBits += num_bits;
			m_NumBitsWritten += num_bits;

			if (m_ScratchBits >= 32)
			{
				uint32_t* ptr = m_Buffer + m_WordIndex;
				uint32_t ptr_value = static_cast<uint32_t>(m_Scratch >> 32);
				*ptr = utility::endian_swap_32(ptr_value);
				m_Scratch <<= 32;
				m_ScratchBits -= 32;
				m_WordIndex++;
			}

			return true;
		}

		bool serialize_bytes(const uint8_t* bytes, uint32_t num_bits)
		{
			uint32_t num_bytes = (num_bits - 1) / 8 + 1;
			for (uint32_t i = 0; i < num_bytes; i++)
			{
				uint32_t value = (uint32_t) * (bytes + i);
				if (!serialize_bits(value, (std::min)(num_bits - i * 8, (uint32_t)8)))
					return false;
			}

			return true;
		}

		bool serialize_bytes_aligned(const uint8_t* bytes, uint32_t num_bits)
		{
			// TODO: Use num_bytes instead of num_bits

			if (!align())
				return false;

			if (!can_write_bits(num_bits))
				return false;

			// Serialize the first bits normally
			uint32_t num_bytes = (num_bits - 1U) / 8U + 1U;

			uint32_t remaining_bits = num_bits;
			uint32_t first_size = (32U - m_ScratchBits) / 8U;
			uint32_t last_size = (num_bytes - first_size - 1U) % 4U + 1U;

			if (!serialize_sequence(bytes, (std::min)(num_bytes, first_size), (std::min)(remaining_bits, 32U - m_ScratchBits))) // TODO: Is the 32 - scratchbits necessary?
				return false;

			// Early exit if we ran out of bits
			if (m_ScratchBits > 0)
				return true;

			remaining_bits -= first_size * 8U;

			// If we have a lot of bits in the middle, just memcpy
			if (remaining_bits >= 32U)
			{
				// Must be a multiple of 4
				uint32_t middle_size = (remaining_bits - 1) / 8U + 1 - last_size;

				std::memcpy(m_Buffer + m_WordIndex, bytes + first_size, middle_size); // TODO: Shouldn't use size, but something smaller

				remaining_bits -= middle_size * 8U;

				m_NumBitsWritten += middle_size * 8U;
				m_WordIndex += middle_size / 4;
			}

			// Serialize the last bits
			if (!serialize_sequence(bytes + num_bytes - last_size, last_size, remaining_bits))
				return false;

			return true;
		}

		bool serialize_into(bit_writer& writer)
		{
			uint8_t* buffer = reinterpret_cast<uint8_t*>(m_Buffer);
			uint32_t num_bits = get_num_bits_written();
			uint32_t remainder_bits = num_bits % 8;

			if (!writer.serialize_bytes(buffer, num_bits - remainder_bits))
				return false;

			if (remainder_bits > 0)
			{
				uint32_t byte_value = buffer[num_bits / 8] >> (8 - remainder_bits);
				if (!writer.serialize_bits(byte_value, remainder_bits))
					return false;
			}

			return true;
		}

		template<typename Trait, typename... Args>
		bool serialize(Args&&... args)
		{
			return serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...);
		}

	private:
		bool serialize_sequence(const uint8_t* bytes, int num_bytes_in_word, int num_bits)
		{
			for (int i = 0; i < num_bytes_in_word; i++)
			{
				uint32_t byte_value = static_cast<uint32_t>(bytes[i]);
				if (!serialize_bits(byte_value, (std::min)(num_bits - i * 8, 8)))
					return false;
			}

			return true;
		}

	private:
		uint32_t* m_Buffer;
		uint32_t m_NumBitsWritten;
		uint32_t m_TotalBits;

		uint64_t m_Scratch;
		uint32_t m_ScratchBits;
		uint32_t m_WordIndex;
	};
}