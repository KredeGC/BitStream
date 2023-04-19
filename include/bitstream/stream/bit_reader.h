#pragma once
#include "../utility/assert.h"
#include "../utility/crc.h"
#include "../utility/endian.h"
#include "../utility/meta.h"

#include "byte_buffer.h"
#include "serialize_traits.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace bitstream
{
	/**
	 * @brief A stream for reading objects from a tightly packed buffer
	 * @note Does not take ownership of the buffer
	*/
	class bit_reader
	{
	public:
		static constexpr bool writing = false;
		static constexpr bool reading = true;

		/**
		 * @brief Default construct a reader pointing to a null buffer
		*/
		bit_reader() noexcept :
			m_Buffer(nullptr),
			m_NumBitsRead(0),
			m_TotalBits(0),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		/**
		 * @brief Construct a reader pointing to the given byte array with @p num_bits
		 * @param bytes The byte array to read from. Should be 4-byte aligned if possible. The size of the array must be a multiple of 4
		 * @param num_bits The maximum number of bits that we can read
		*/
		explicit bit_reader(const void* bytes, uint32_t num_bits) noexcept :
			m_Buffer(static_cast<const uint32_t*>(bytes)),
			m_NumBitsRead(0),
			m_TotalBits(num_bits),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		/**
		 * @brief Construct a reader pointing to the given @p buffer
		 * @param buffer The buffer to read from
		 * @param num_bits The maximum number of bits that we can read
		*/
		template<size_t Size>
		explicit bit_reader(byte_buffer<Size>& buffer, uint32_t num_bits) noexcept :
			m_Buffer(reinterpret_cast<uint32_t*>(buffer.Bytes)),
			m_NumBitsRead(0),
			m_TotalBits(num_bits),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

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

		bit_reader& operator=(const bit_reader&) = delete;

		bit_reader& operator=(bit_reader&& rhs) noexcept
		{
			m_Buffer = rhs.m_Buffer;
			m_NumBitsRead = rhs.m_NumBitsRead;
			m_TotalBits = rhs.m_TotalBits;
			m_Scratch = rhs.m_Scratch;
			m_ScratchBits = rhs.m_ScratchBits;
			m_WordIndex = rhs.m_WordIndex;

			rhs.m_Buffer = nullptr;
			rhs.m_NumBitsRead = 0;
			rhs.m_TotalBits = 0;
			rhs.m_Scratch = 0;
			rhs.m_ScratchBits = 0;
			rhs.m_WordIndex = 0;
            
            return *this;
		}

		/**
		 * @brief Returns the buffer that this reader is currently serializing from
		 * @return The buffer
		*/
		const uint8_t* get_buffer() const noexcept { return reinterpret_cast<const uint8_t*>(m_Buffer); }

		/**
		 * @brief Returns the number of bits which have been read from the buffer
		 * @return The number of bits which have been read
		*/
		uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsRead; }

		/**
		 * @brief Returns the number of bytes which have been read from the buffer
		 * @return The number of bytes which have been read
		*/
		uint32_t get_num_bytes_serialized() const noexcept { return m_NumBitsRead > 0U ? ((m_NumBitsRead - 1U) / 8U + 1U) : 0U; }

		/**
		 * @brief Returns whether the @p num_bits be read from the buffer
		 * @param num_bits The number of bits to test
		 * @return Whether the number of bits can be read from the buffer
		*/
		bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_NumBitsRead + num_bits <= m_TotalBits; }

		/**
		 * @brief Returns the number of bits which have not been read yet
		 * @note The same as get_total_bits() - get_num_bits_serialized()
		 * @return The remaining space in the buffer
		*/
		uint32_t get_remaining_bits() const noexcept { return m_TotalBits - m_NumBitsRead; }

		/**
		 * @brief Returns the size of the buffer, in bits
		 * @return The size of the buffer, in bits
		*/
		uint32_t get_total_bits() const noexcept { return m_TotalBits; }

		/**
		 * @brief Reads the first 32 bits of the buffer and compares it to a checksum of the @p protocol_version and the rest of the buffer
		 * @param protocol_version A unique version number
		 * @return Whether the checksum matches what was written
		*/
		bool serialize_checksum(uint32_t protocol_version) noexcept
		{
			BS_ASSERT(m_NumBitsRead == 0);

			BS_ASSERT(can_serialize_bits(32U));

			uint32_t num_bytes = (m_TotalBits - 1U) / 8U + 1U;

			// Generate checksum to compare against
			uint32_t generated_checksum = utility::crc_uint32(reinterpret_cast<const uint8_t*>(&protocol_version), reinterpret_cast<const uint8_t*>(m_Buffer + 1), num_bytes - 4);

			// Advance the reader by the size of the checksum (32 bits / 1 word)
			m_WordIndex++;
			m_NumBitsRead += 32U;

			// Read the checksum
			uint32_t checksum = *m_Buffer;

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

			uint32_t remainder = (num_bytes * 8U - m_NumBitsRead) % 32U;
			uint32_t zero;

			// Test the last word more carefully, as it may have data
			if (remainder != 0U)
			{
				bool status = serialize_bits(zero, remainder);
				BS_ASSERT(status && zero == 0);
			}

			uint32_t offset = m_NumBitsRead / 32;
            uint32_t max = num_bytes / 4;

			// Test for zeros in padding
			for (uint32_t i = offset; i < max; i++)
			{
				bool status = serialize_bits(zero, 32);
				BS_ASSERT(status && zero == 0);
			}

			return true;
		}

		/**
		 * @brief Pads the buffer with up to 8 zeros, so that the next read is byte-aligned
		 * @notes Return false if the padded bits are not zeros
		 * @return Returns false if the padded bits are not zeros
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
		 * @brief Reads from the buffer, using the given @p Trait.
		 * @note The Trait type in this function must always be explicitly declared
		 * @tparam Trait A template specialization of serialize_trait<>
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param ...args The arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename Trait, typename... Args>
		bool serialize(Args&&... args) noexcept(utility::is_noexcept_serialize_v<Trait, bit_reader, Args...>)
		{
			static_assert(utility::has_serialize_v<Trait, bit_reader, Args...>, "Could not find serializable trait for the given type. Remember to specialize serializable_traits<> with the given type");

			return serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...);
		}

		/**
		 * @brief Reads from the buffer, by trying to deduce the trait.
		 * @note The Trait type in this function is always implicit and will be deduced from the first argument if possible.
		 * If the trait cannot be deduced it will not compile.
		 * @tparam Trait The type of the first argument, which will be used to deduce the trait specialization
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param arg The first argument to pass to the serialize function
		 * @param ...args The rest of the arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename Trait, typename... Args>
		bool serialize(Trait&& arg, Args&&... args) noexcept(utility::is_noexcept_serialize_v<utility::deduce_trait_t<Trait, bit_reader, Args...>, bit_reader, Trait, Args...>)
		{
			using deduce_t = utility::deduce_trait_t<Trait, bit_reader, Args...>;

			static_assert(utility::has_serialize_v<deduce_t, bit_reader, Trait, Args...>, "Could not deduce serializable trait for the given arguments. Remember to specialize serializable_traits<> with the given type");

			return serialize_traits<deduce_t>::serialize(*this, std::forward<Trait>(arg), std::forward<Args>(args)...);
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