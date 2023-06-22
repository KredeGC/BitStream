#pragma once
#include "../utility/assert.h"
#include "../utility/crc.h"
#include "../utility/endian.h"
#include "../utility/meta.h"

#include "byte_buffer.h"
#include "serialize_traits.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace bitstream
{
    class bit_reader;
    
	/**
	 * @brief A stream for writing objects tightly into a buffer
	 * @note Does not take ownership of the buffer
	*/
	class bit_writer
	{
    private:
        friend class bit_reader;
        
	public:
		static constexpr bool writing = true;
		static constexpr bool reading = false;

		/**
		 * @brief Default construct a writer pointing to a null buffer
		*/
		bit_writer() noexcept :
			m_Buffer(nullptr),
			m_NumBitsWritten(0),
			m_TotalBits(0),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		/**
		 * @brief Construct a writer pointing to the given byte array with @p num_bytes size
		 * @param bytes The byte array to write to. Must be 4-byte aligned and the size must be a multiple of 4
		 * @param num_bytes The number of bytes in the array
		*/
		explicit bit_writer(void* bytes, uint32_t num_bytes) noexcept :
			m_Buffer(static_cast<uint32_t*>(bytes)),
			m_NumBitsWritten(0),
			m_TotalBits(num_bytes * 8),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		/**
		 * @brief Construct a writer pointing to the given @p buffer
		 * @param buffer The buffer to write to
		*/
		template<size_t Size>
		explicit bit_writer(byte_buffer<Size>& buffer) noexcept :
			m_Buffer(reinterpret_cast<uint32_t*>(buffer.Bytes)),
			m_NumBitsWritten(0),
			m_TotalBits(Size * 8),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

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

		bit_writer& operator=(const bit_writer&) = delete;

		bit_writer& operator=(bit_writer&& rhs) noexcept
		{
			m_Buffer = rhs.m_Buffer;
			m_NumBitsWritten = rhs.m_NumBitsWritten;
			m_TotalBits = rhs.m_TotalBits;
			m_Scratch = rhs.m_Scratch;
			m_ScratchBits = rhs.m_ScratchBits;
			m_WordIndex = rhs.m_WordIndex;

			rhs.m_Buffer = nullptr;
			rhs.m_NumBitsWritten = 0;
			rhs.m_TotalBits = 0;
			rhs.m_Scratch = 0;
			rhs.m_ScratchBits = 0;
			rhs.m_WordIndex = 0;
            
            return *this;
		}

		/**
		 * @brief Returns the buffer that this writer is currently serializing into
		 * @return The buffer
		*/
		[[nodiscard]] uint8_t* get_buffer() const noexcept { return reinterpret_cast<uint8_t*>(m_Buffer); }

		/**
		 * @brief Returns the number of bits which have been written to the buffer
		 * @return The number of bits which have been written
		*/
		[[nodiscard]] uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsWritten; }

		/**
		 * @brief Returns the number of bytes which have been written to the buffer
		 * @return The number of bytes which have been written
		*/
		[[nodiscard]] uint32_t get_num_bytes_serialized() const noexcept { return m_NumBitsWritten > 0U ? ((m_NumBitsWritten - 1U) / 8U + 1U) : 0U; }

		/**
		 * @brief Returns whether the @p num_bits can fit in the buffer
		 * @param num_bits The number of bits to test
		 * @return Whether the number of bits can fit in the buffer
		*/
		[[nodiscard]] bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_NumBitsWritten + num_bits <= m_TotalBits; }

		/**
		 * @brief Returns the number of bits which have not been written yet
		 * @note The same as get_total_bits() - get_num_bits_serialized()
		 * @return The remaining space in the buffer
		*/
		[[nodiscard]] uint32_t get_remaining_bits() const noexcept { return m_TotalBits - m_NumBitsWritten; }

        /**
         * @brief Returns the size of the buffer, in bits
         * @return The size of the buffer, in bits
        */
		[[nodiscard]] uint32_t get_total_bits() const noexcept { return m_TotalBits; }
        
		/**
		 * @brief Flushes any remaining bits into the buffer. Use this when you no longer intend to write anything to the buffer.
		 * @return The number of bytes written to the buffer
		*/
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

			return m_NumBitsWritten;
		}

		/**
		 * @brief Instructs the writer that you intend to use `serialize_checksum()` later on, and to reserve the first 32 bits.
		 * @return Returns false if anything has already been written to the buffer or if there's no space to write the checksum
		*/
		[[nodiscard]] bool prepend_checksum() noexcept
		{
			BS_ASSERT(m_NumBitsWritten == 0);

            BS_ASSERT(can_serialize_bits(32U));
            
			// Advance the reader by the size of the checksum (32 bits / 1 word)
			m_WordIndex++;
			m_NumBitsWritten += 32U;
            
            return true;
		}

		/**
		 * @brief Writes a checksum of the @p protocol_version and the rest of the buffer as the first 32 bits
		 * @param protocol_version A unique version number
		 * @return The number of bytes written to the buffer
		*/
		uint32_t serialize_checksum(uint32_t protocol_version) noexcept
		{
			uint32_t num_bits = flush();

			// Copy protocol version to buffer
			*m_Buffer = protocol_version;

			// Generate checksum of version + data
			uint32_t checksum = utility::crc_uint32(reinterpret_cast<uint8_t*>(m_Buffer), get_num_bytes_serialized());

			// Put checksum at beginning
			*m_Buffer = checksum;

			return num_bits;
		}

		/**
		 * @brief Pads the buffer up to the given number of bytes with zeros
		 * @param num_bytes The byte number to pad to
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes
		*/
		[[nodiscard]] bool pad_to_size(uint32_t num_bytes) noexcept
		{
			BS_ASSERT(num_bytes * 8U <= m_TotalBits);

			BS_ASSERT(num_bytes * 8U >= m_NumBitsWritten);
            
            if (m_NumBitsWritten == 0)
            {
                std::memset(m_Buffer, 0, num_bytes);
                
                m_NumBitsWritten = num_bytes * 8;
                m_Scratch = 0;
                m_ScratchBits = 0;
                m_WordIndex = num_bytes / 4;
                
                return true;
            }
            
            uint32_t remainder = (num_bytes * 8U - m_NumBitsWritten) % 32U;
			uint32_t zero = 0;

			// Align to byte
			if (remainder != 0U)
				BS_ASSERT(serialize_bits(zero, remainder));

			uint32_t offset = m_NumBitsWritten / 32;
            uint32_t max = num_bytes / 4;

			// Serialize words
			for (uint32_t i = offset; i < max; i++)
				BS_ASSERT(serialize_bits(zero, 32));

			return true;
		}

		/**
		 * @brief Pads the buffer with up to 8 zeros, so that the next write is byte-aligned
		 * @return Success
		*/
		[[nodiscard]] bool align() noexcept
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

		/**
		 * @brief Writes the first @p num_bits bits of @p value into the buffer
		 * @param value The value to serialize
		 * @param num_bits The number of bits of the @p value to serialize
		 * @return Returns false if @p num_bits is less than 1 or greater than 32 or if writing the given number of bits would overflow the buffer
		*/
		[[nodiscard]] bool serialize_bits(uint32_t value, uint32_t num_bits) noexcept
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

		/**
		 * @brief Writes the first @p num_bits bits of the given byte array, 32 bits at a time
		 * @param bytes The bytes to serialize
		 * @param num_bits The number of bits of the @p bytes to serialize
		 * @return Returns false if @p num_bits is less than 1 or if writing the given number of bits would overflow the buffer
		*/
		[[nodiscard]] bool serialize_bytes(const uint8_t* bytes, uint32_t num_bits) noexcept
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

		/**
		 * @brief Writes the contents of the buffer into the given @p writer. Essentially copies the entire buffer without modifying it.
		 * @param writer The writer to copy into
		 * @return Returns false if writing would overflow the buffer
		*/
		[[nodiscard]] bool serialize_into(bit_writer& writer) const noexcept
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

		/**
		 * @brief Writes to the buffer, using the given @p Trait.
		 * @note The Trait type in this function must always be explicitly declared
		 * @tparam Trait A template specialization of serialize_trait<>
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param ...args The arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename Trait, typename... Args, typename = utility::has_serialize_t<Trait, bit_writer, Args...>>
		[[nodiscard]] bool serialize(Args&&... args) noexcept(utility::is_serialize_noexcept_v<Trait, bit_writer, Args...>)
		{
			return serialize_traits<Trait>::serialize(*this, std::forward<Args>(args)...);
		}

		/**
		 * @brief Writes to the buffer, by trying to deduce the trait.
		 * @note The Trait type in this function is always implicit and will be deduced from the first argument if possible.
		 * If the trait cannot be deduced it will not compile.
		 * @tparam Trait The type of the first argument, which will be used to deduce the trait specialization
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param arg The first argument to pass to the serialize function
		 * @param ...args The rest of the arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename... Args, typename Trait, typename = utility::has_deduce_serialize_t<Trait, bit_writer, Args...>>
		[[nodiscard]] bool serialize(Trait&& arg, Args&&... args) noexcept(utility::is_deduce_serialize_noexcept_v<Trait, bit_writer, Args...>)
		{
			return serialize_traits<utility::deduce_trait_t<Trait, bit_writer, Args...>>::serialize(*this, std::forward<Trait>(arg), std::forward<Args>(args)...);
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