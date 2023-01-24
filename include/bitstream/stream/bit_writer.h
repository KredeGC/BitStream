#pragma once
#include "../utility/crc.h"
#include "../utility/endian.h"

#include <cstdint>
#include <memory>

namespace bitstream::stream
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
			m_WordIndex(0) {}

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
			flush();

			if (size * 8 < m_NumBitsWritten)
				return false;

			m_NumBitsWritten = size * 8;
			m_WordIndex = (m_NumBitsWritten - 1) / 32 + 1; // Might be wrong?

			return true;
		}

		bool align()
		{
			const uint32_t remainder = m_ScratchBits % 8;
			if (remainder != 0)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, 8 - remainder);

				return status && m_NumBitsWritten % 8 == 0;
			}
			return true;
		}

		bool serialize_bits(uint32_t& value, uint32_t num_bits)
		{
			BS_ASSERT_RETURN(num_bits > 0 && num_bits <= 32);

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

		template<typename Trait, typename... Args>
		bool serialize(Args&&... args)
		{
			return serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...);
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