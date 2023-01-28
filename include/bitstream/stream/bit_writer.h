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
    class bit_reader;
    
	class bit_writer
	{
    private:
        friend class bit_reader;
        
	public:
		static constexpr bool writing = true;
		static constexpr bool reading = false;

		bit_writer() noexcept :
			m_Buffer(nullptr),
			m_NumBitsWritten(0),
			m_TotalBits(0),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		bit_writer(void* bytes, uint32_t num_bytes) noexcept :
			m_Buffer(static_cast<uint32_t*>(bytes)),
			m_NumBitsWritten(0),
			m_TotalBits(num_bytes * 8),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0)
		{
#ifdef BS_DEBUG_BREAK
			if (num_bytes % 4 != 0)
                BS_BREAKPOINT();
#endif
		}

		bit_writer(const bit_writer&) = delete;
        
		bit_writer(bit_writer&& other) noexcept :
            m_Buffer(other.m_Buffer),
            m_NumBitsWritten(other.m_NumBitsWritten),
            m_TotalBits(other.m_TotalBits),
            m_Scratch(other.m_Scratch),
            m_ScratchBits(other.m_ScratchBits),
            m_WordIndex(other.m_WordIndex)
        {
            other.m_Buffer = nullptr;
            other.m_NumBitsWritten = 0;
            other.m_TotalBits = 0;
            other.m_Scratch = 0;
            other.m_ScratchBits = 0;
            other.m_WordIndex = 0;
        }

		uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsWritten; }

		bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_NumBitsWritten + num_bits <= m_TotalBits; }

		uint32_t get_remaining_bits() const noexcept { return m_TotalBits - m_NumBitsWritten; }

        uint32_t get_total_bits() const noexcept { return m_TotalBits; }
        
		uint32_t flush() noexcept
		{
			if (m_ScratchBits > 0U)
			{
				uint32_t* ptr = m_Buffer + m_WordIndex;
				uint32_t ptr_value = static_cast<uint32_t>(m_Scratch >> 32U);
				*ptr = utility::endian_swap_32(ptr_value);

				m_Scratch = 0U;
				m_ScratchBits = 0U;
				m_WordIndex++;
			}

			return (m_NumBitsWritten - 1U) / 8U + 1U;
		}

		bool prepend_checksum() noexcept
		{
            BS_ASSERT(can_serialize_bits(32U));
            
			// Advance the reader by the size of the checksum (32 bits / 1 word)
			m_WordIndex++;
			m_NumBitsWritten += 32U;
            
            return true;
		}

		uint32_t serialize_checksum(uint32_t protocol_version) noexcept
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

		bool pad_to_size(uint32_t size) noexcept
		{
			BS_ASSERT(size * 8U <= m_TotalBits);

			flush();

			BS_ASSERT(size * 8U >= m_NumBitsWritten);

			// Set to the padding to 0
			std::memset(m_Buffer + m_WordIndex, 0, size - m_WordIndex * 4U);

			m_NumBitsWritten = size * 8U;

			m_Scratch = 0ULL;
			m_ScratchBits = (size % 4U) * 8U;
			m_WordIndex = size / 4U;

			return true;
		}

		bool align() noexcept
		{
			uint32_t remainder = m_ScratchBits % 8U;
			if (remainder != 0U)
			{
				uint32_t zero = 0U;
				bool status = serialize_bits(zero, 8U - remainder);

				BS_ASSERT(status && m_NumBitsWritten % 8U == 0U);
			}
			return true;
		}

		bool serialize_bits(uint32_t value, uint32_t num_bits) noexcept
		{
			BS_ASSERT(num_bits > 0U && num_bits <= 32U);

			BS_ASSERT(can_serialize_bits(num_bits));

			uint32_t offset = 64U - num_bits - m_ScratchBits;
			uint64_t ls_value = static_cast<uint64_t>(value) << offset;

			m_Scratch |= ls_value;
			m_ScratchBits += num_bits;
			m_NumBitsWritten += num_bits;

			if (m_ScratchBits >= 32U)
			{
				uint32_t* ptr = m_Buffer + m_WordIndex;
				uint32_t ptr_value = static_cast<uint32_t>(m_Scratch >> 32U);
				*ptr = utility::endian_swap_32(ptr_value);
				m_Scratch <<= 32ULL;
				m_ScratchBits -= 32U;
				m_WordIndex++;
			}

			return true;
		}

		bool serialize_bytes(const uint8_t* bytes, uint32_t num_bits) noexcept
		{
			BS_ASSERT(num_bits > 0U);
            
			BS_ASSERT(can_serialize_bits(num_bits));
            
            // Write the byte array as words
            const uint32_t* word_buffer = reinterpret_cast<const uint32_t*>(bytes);
			uint32_t num_words = num_bits / 32U;
            
            if (m_ScratchBits % 32U == 0U && num_words > 0U)
            {
                // If the written buffer is word-aligned, just memcpy it
                std::memcpy(m_Buffer + m_WordIndex, word_buffer, num_words * 4U);
                
                m_NumBitsWritten += num_words * 32U;
                m_WordIndex += num_words;
            }
            else
            {
                // If the buffer is not word-aligned, serialize a word at a time
                for (uint32_t i = 0U; i < num_words; i++)
                {
                    // Casting a byte-array to an int is wrong on little-endian systems
                    // We have to swap the bytes around
                    uint32_t value = utility::endian_swap_32(word_buffer[i]);
                    BS_ASSERT(serialize_bits(value, 32U));
                }
            }
            
            // Early exit if the word-count matches
            if (num_bits % 32U == 0U)
                return true;
            
            uint32_t remaining_bits = num_bits - num_words * 32U;
            
            uint32_t num_bytes = (remaining_bits - 1U) / 8U + 1U;
			for (uint32_t i = 0U; i < num_bytes; i++)
			{
				uint32_t value = static_cast<uint32_t>(bytes[num_words * 4U + i]);
				BS_ASSERT(serialize_bits(value, (std::min)(remaining_bits - i * 8U, 8U)));
			}

			return true;
		}

		bool serialize_into(bit_writer& writer) const noexcept
		{
			uint8_t* buffer = reinterpret_cast<uint8_t*>(m_Buffer);
			uint32_t num_bits = get_num_bits_serialized();
			uint32_t remainder_bits = num_bits % 8U;

			BS_ASSERT(writer.serialize_bytes(buffer, num_bits - remainder_bits));

			if (remainder_bits > 0U)
			{
				uint32_t byte_value = buffer[num_bits / 8U] >> (8U - remainder_bits);
				BS_ASSERT(writer.serialize_bits(byte_value, remainder_bits));
			}

			return true;
		}

		template<typename Trait, typename... Args>
		bool serialize(Args&&... args) noexcept(noexcept(serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...)))
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