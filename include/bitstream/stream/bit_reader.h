#pragma once
#include "../utility/assert.h"
#include "../utility/crc.h"
#include "../utility/endian.h"

#include "bit_writer.h"
#include "serialize_traits.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace bitstream
{
	class bit_reader
	{
	public:
		static constexpr bool writing = false;
		static constexpr bool reading = true;

		bit_reader() noexcept :
			m_Buffer(nullptr),
			m_NumBitsRead(0),
			m_TotalBits(0),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		bit_reader(const void* bytes, uint32_t num_bytes) noexcept :
			m_Buffer(static_cast<const uint32_t*>(bytes)),
			m_NumBitsRead(0),
			m_TotalBits(num_bytes * 8),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}
        
		explicit bit_reader(bit_writer&& other) noexcept :
            m_Buffer(other.m_Buffer),
            m_NumBitsRead(0),
            m_TotalBits(other.m_NumBitsWritten),
            m_Scratch(0),
            m_ScratchBits(0),
            m_WordIndex(0)
        {
            other.m_Buffer = nullptr;
            other.m_NumBitsWritten = 0;
            other.m_TotalBits = 0;
            other.m_Scratch = 0;
            other.m_ScratchBits = 0;
            other.m_WordIndex = 0;
        }

		bit_reader(const bit_reader&) = delete;
        
		bit_reader(bit_reader&& other) noexcept :
            m_Buffer(other.m_Buffer),
            m_NumBitsRead(other.m_NumBitsRead),
            m_TotalBits(other.m_TotalBits),
            m_Scratch(other.m_Scratch),
            m_ScratchBits(other.m_ScratchBits),
            m_WordIndex(other.m_WordIndex)
        {
            other.m_Buffer = nullptr;
            other.m_NumBitsRead = 0;
            other.m_TotalBits = 0;
            other.m_Scratch = 0;
            other.m_ScratchBits = 0;
            other.m_WordIndex = 0;
        }

		uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsRead; }

		bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_NumBitsRead + num_bits <= m_TotalBits; }

		uint32_t get_remaining_bits() const noexcept { return m_TotalBits - m_NumBitsRead; }
        
        uint32_t get_total_bits() const noexcept { return m_TotalBits; }

		/**
		 * @brief Reads the first 32 bits of the buffer and compares it to a checksum of the @p protocol_version and the rest of the buffer
		 * @param protocol_version A unique version number
		 * @return Whether the checksum matches what was written
		*/
		bool serialize_checksum(uint32_t protocol_version) noexcept
		{
			uint32_t num_bytes = (m_TotalBits - 1U) / 8U + 1U;

			// Read the checksum
			uint32_t checksum;
			std::memcpy(&checksum, m_Buffer, sizeof(uint32_t));

			// Copy protocol version to buffer
			uint32_t* buffer = const_cast<uint32_t*>(m_Buffer); // Somewhat of a hack, but it's faster to change the checksum twice than allocate memory for it
			std::memcpy(buffer, &protocol_version, sizeof(uint32_t));

			// Generate checksum to compare against
			uint32_t generated_checksum = utility::crc_uint32(reinterpret_cast<uint8_t*>(buffer), num_bytes);

			// Write the checksum back, just in case
			std::memcpy(buffer, &checksum, sizeof(uint32_t));

			// Advance the reader by the size of the checksum (32 bits / 1 word)
			m_WordIndex++;
			m_NumBitsRead += 32U;

			// Compare the checksum
			return generated_checksum == checksum;
		}

		/**
		 * @brief Pads the buffer up to the given number of bytes
		 * @param num_bytes The byte number to pad to
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes or if the padded bits are not zeros.
		*/
		bool pad_to_size(uint32_t num_bytes) noexcept
		{
			BS_ASSERT(num_bytes * 8U <= m_TotalBits);
            
			BS_ASSERT(num_bytes * 8U >= m_NumBitsRead);

			// Align with word size
			uint32_t remainder = m_NumBitsRead % 32;
			if (remainder != 0)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 32 - remainder);

				BS_ASSERT(status && zero == 0);
			}

			// Test for zeros in padding
			for (uint32_t i = m_WordIndex; i < num_bytes / 4; i++)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, 32);

				BS_ASSERT(status && zero == 0);
			}

			// Test the last word more carefully, as it may have data
			if (num_bytes % 4 != 0)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, (num_bytes % 4) * 8);

				BS_ASSERT(status && zero == 0);
			}

			return true;
		}

		/**
		 * @brief Pads the buffer with up to 8 zeros, so that the next read is byte-aligned
		 * @notes Return false if the padded bits are not zeros
		 * @return Success
		*/
		bool align() noexcept
		{
			uint32_t remainder = m_NumBitsRead % 8U;
			if (remainder != 0U)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 8U - remainder);

                BS_ASSERT(status && zero == 0U && m_NumBitsRead % 8U == 0U);
			}

			return true;
		}

		/**
		 * @brief Reads the first @p num_bits bits of @p value from the buffer
		 * @param value The value to serialize
		 * @param num_bits The number of bits of the @p value to serialize
		 * @return Returns false if @p num_bits is less than 1 or greater than 32 or if reading the given number of bits would overflow the buffer
		*/
		bool serialize_bits(uint32_t& value, uint32_t num_bits) noexcept
		{
			BS_ASSERT(num_bits > 0U && num_bits <= 32U);

			BS_ASSERT(can_serialize_bits(num_bits));

			if (m_ScratchBits < num_bits)
			{
				const uint32_t* ptr = m_Buffer + m_WordIndex;

				uint64_t ptr_value = static_cast<uint64_t>(utility::endian_swap_32(*ptr)) << (32U - m_ScratchBits);
				m_Scratch |= ptr_value;
				m_ScratchBits += 32U;
				m_WordIndex++;
			}

			uint32_t offset = 64U - num_bits;
			value = static_cast<uint32_t>(m_Scratch >> offset);

			m_Scratch <<= num_bits;
			m_ScratchBits -= num_bits;
			m_NumBitsRead += num_bits;

			return true;
		}

		/**
		 * @brief Reads the first @p num_bits bits of the given byte array, 32 bits at a time
		 * @param bytes The bytes to serialize
		 * @param num_bits The number of bits of the @p bytes to serialize
		 * @return Returns false if @p num_bits is less than 1 or if reading the given number of bits would overflow the buffer
		*/
		bool serialize_bytes(uint8_t* bytes, uint32_t num_bits) noexcept
		{
			BS_ASSERT(num_bits > 0U);
            
			BS_ASSERT(can_serialize_bits(num_bits));
            
            // Read the byte array as words
            uint32_t* word_buffer = reinterpret_cast<uint32_t*>(bytes);
			uint32_t num_words = num_bits / 32U;
            
            if (m_ScratchBits % 32U == 0U && num_words > 0U)
            {
                // If the read buffer is word-aligned, just memcpy it
                std::memcpy(word_buffer, m_Buffer + m_WordIndex, num_words * 4U);
                
                m_NumBitsRead += num_words * 32U;
                m_WordIndex += num_words;
            }
            else
            {
                // If the buffer is not word-aligned, serialize a word at a time
                for (uint32_t i = 0U; i < num_words; i++)
                {
                    uint32_t value;
                    BS_ASSERT(serialize_bits(value, 32U));
                    
                    // Casting a byte-array to an int is wrong on little-endian systems
                    // We have to swap the bytes around
                    word_buffer[i] = utility::endian_swap_32(value);
                }
            }
            
            // Early exit if the word-count matches
            if (num_bits % 32 == 0)
                return true;
            
            uint32_t remaining_bits = num_bits - num_words * 32U;
            
            uint32_t num_bytes = (remaining_bits - 1U) / 8U + 1U;
			for (uint32_t i = 0; i < num_bytes; i++)
			{
                uint32_t value;
				BS_ASSERT(serialize_bits(value, (std::min)(remaining_bits - i * 8U, 8U)));
                
				bytes[num_words * 4 + i] = static_cast<uint8_t>(value);
			}

			return true;
		}

		/**
		 * @brief Reads from the buffer, using the given `Trait`.
		 * @tparam Trait A template specialization of serialize_trait<>
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param ...args The arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename Trait, typename... Args>
		bool serialize(Args&&... args) noexcept(noexcept(serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...)))
		{
			return serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...);
		}

	private:
		const uint32_t* m_Buffer;
		uint32_t m_NumBitsRead;
		uint32_t m_TotalBits;

		uint64_t m_Scratch;
		uint32_t m_ScratchBits;
		uint32_t m_WordIndex;
	};
}