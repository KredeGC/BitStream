#pragma once
#include "../utility/assert.h"
#include "../utility/endian.h"
#include "../utility/meta.h"

#include "byte_buffer.h"
#include "multi.h"
#include "serialize_traits.h"
#include "stream_traits.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace bitstream
{
	/**
	 * @brief A stream for reading objects from a tightly packed buffer
	 * @tparam Policy The underlying representation of the buffer
	*/
	template<typename Policy>
	class bit_reader
	{
	public:
		static constexpr bool writing = false;
		static constexpr bool reading = true;

		/**
		 * @brief Construct a reader with the parameters passed to the underlying policy
		 * @param ...args The arguments to pass to the policy
		*/
		template<typename... Ts,
			typename = std::enable_if_t<std::is_constructible_v<Policy, Ts...>>>
		bit_reader(Ts&&... args)
			noexcept(std::is_nothrow_constructible_v<Policy, Ts...>) :
			m_Policy(std::forward<Ts>(args) ...),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		bit_reader(const bit_reader&) = delete;
        
		bit_reader(bit_reader&& other) noexcept :
			m_Policy(std::move(other.m_Policy)),
            m_Scratch(other.m_Scratch),
            m_ScratchBits(other.m_ScratchBits),
            m_WordIndex(other.m_WordIndex)
        {
            other.m_Scratch = 0;
            other.m_ScratchBits = 0;
            other.m_WordIndex = 0;
        }

		bit_reader& operator=(const bit_reader&) = delete;

		bit_reader& operator=(bit_reader&& rhs) noexcept
		{
			m_Policy = std::move(rhs.m_Policy);
			m_Scratch = rhs.m_Scratch;
			m_ScratchBits = rhs.m_ScratchBits;
			m_WordIndex = rhs.m_WordIndex;

			rhs.m_Scratch = 0;
			rhs.m_ScratchBits = 0;
			rhs.m_WordIndex = 0;
            
            return *this;
		}

		/**
		 * @brief Returns the buffer that this reader is currently serializing from
		 * @return The buffer
		*/
		[[nodiscard]] const uint8_t* get_buffer() const noexcept { return reinterpret_cast<const uint8_t*>(m_Policy.get_buffer()); }

		/**
		 * @brief Returns the number of bits which have been read from the buffer
		 * @return The number of bits which have been read
		*/
		[[nodiscard]] uint32_t get_num_bits_serialized() const noexcept { return m_Policy.get_num_bits_serialized(); }

		/**
		 * @brief Returns the number of bytes which have been read from the buffer
		 * @return The number of bytes which have been read
		*/
		[[nodiscard]] uint32_t get_num_bytes_serialized() const noexcept { return get_num_bits_serialized() > 0U ? ((get_num_bits_serialized() - 1U) / 8U + 1U) : 0U; }

		/**
		 * @brief Returns whether the @p num_bits be read from the buffer
		 * @param num_bits The number of bits to test
		 * @return Whether the number of bits can be read from the buffer
		*/
		[[nodiscard]] bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_Policy.can_serialize_bits(num_bits); }

		/**
		 * @brief Returns the number of bits which have not been read yet
		 * @note The same as get_total_bits() - get_num_bits_serialized()
		 * @return The remaining space in the buffer
		*/
		[[nodiscard]] uint32_t get_remaining_bits() const noexcept { return get_total_bits() - get_num_bits_serialized(); }

		/**
		 * @brief Returns the size of the buffer, in bits
		 * @return The size of the buffer, in bits
		*/
		[[nodiscard]] uint32_t get_total_bits() const noexcept { return m_Policy.get_total_bits(); }

		/**
		 * @brief Pads the buffer up to the given number of bytes
		 * @param num_bytes The byte number to pad to
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes or if the padded bits are not zeros.
		*/
		[[nodiscard]] bool pad_to_size(uint32_t num_bytes) noexcept
		{
			uint32_t num_bits_read = get_num_bits_serialized();

			BS_ASSERT(num_bytes * 8U >= num_bits_read);

			BS_ASSERT(can_serialize_bits(num_bytes * 8U - num_bits_read));

			uint32_t remainder = (num_bytes * 8U - num_bits_read) % 32U;
			uint32_t zero;

			// Test the last word more carefully, as it may have data
			if (remainder != 0U)
			{
				bool status = serialize_bits(zero, remainder);
				BS_ASSERT(status && zero == 0);
			}

			uint32_t offset = get_num_bits_serialized() / 32;
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
		 * @brief Pads the buffer up with the given number of bytes
		 * @param num_bytes The amount of bytes to pad
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes or if the padded bits are not zeros.
		*/
		[[nodiscard]] bool pad(uint32_t num_bytes) noexcept
		{
			return pad_to_size(get_num_bytes_serialized() + num_bytes);
		}

		/**
		 * @brief Pads the buffer with up to 8 zeros, so that the next read is byte-aligned
		 * @notes Return false if the padded bits are not zeros
		 * @return Returns false if the padded bits are not zeros
		*/
		[[nodiscard]] bool align() noexcept
		{
			uint32_t remainder = get_num_bits_serialized() % 8U;
			if (remainder != 0U)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 8U - remainder);

                BS_ASSERT(status && zero == 0U && get_num_bits_serialized() % 8U == 0U);
			}

			return true;
		}

		/**
		 * @brief Reads the first @p num_bits bits of @p value from the buffer
		 * @param value The value to serialize
		 * @param num_bits The number of bits of the @p value to serialize
		 * @return Returns false if @p num_bits is less than 1 or greater than 32 or if reading the given number of bits would overflow the buffer
		*/
		[[nodiscard]] bool serialize_bits(uint32_t& value, uint32_t num_bits) noexcept
		{
			BS_ASSERT(num_bits > 0U && num_bits <= 32U);

			BS_ASSERT(m_Policy.extend(num_bits));

			// This is actually slower
			// Possibly due to unlikely branching
			/*if (num_bits == 32U && m_ScratchBits == 0U)
			{
				const uint32_t* ptr = m_Policy.get_buffer() + m_WordIndex;

				value = utility::to_big_endian32(*ptr);

				m_WordIndex++;

				return true;
			}*/

			if (m_ScratchBits < num_bits)
			{
				const uint32_t* ptr = m_Policy.get_buffer() + m_WordIndex;

				uint64_t ptr_value = static_cast<uint64_t>(utility::to_big_endian32(*ptr)) << (32U - m_ScratchBits);
				m_Scratch |= ptr_value;
				m_ScratchBits += 32U;
				m_WordIndex++;
			}

			uint32_t offset = 64U - num_bits;
			value = static_cast<uint32_t>(m_Scratch >> offset);

			m_Scratch <<= num_bits;
			m_ScratchBits -= num_bits;

			return true;
		}

		/**
		 * @brief Reads the first @p num_bits bits of the given byte array, 32 bits at a time
		 * @param bytes The bytes to serialize
		 * @param num_bits The number of bits of the @p bytes to serialize
		 * @return Returns false if @p num_bits is less than 1 or if reading the given number of bits would overflow the buffer
		*/
		[[nodiscard]] bool serialize_bytes(uint8_t* bytes, uint32_t num_bits) noexcept
		{
			BS_ASSERT(num_bits > 0U);
            
			BS_ASSERT(can_serialize_bits(num_bits));
            
            // Read the byte array as words
            uint32_t* word_buffer = reinterpret_cast<uint32_t*>(bytes);
			uint32_t num_words = num_bits / 32U;
            
            if (m_ScratchBits % 32U == 0U && num_words > 0U)
            {
				BS_ASSERT(m_Policy.extend(num_words * 32U));

                // If the read buffer is word-aligned, just memcpy it
                std::memcpy(word_buffer, m_Policy.get_buffer() + m_WordIndex, num_words * 4U);
                
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
                    word_buffer[i] = utility::to_big_endian32(value);
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

		template<typename... Args, typename = std::enable_if_t<(utility::has_instance_serialize_v<Args, bit_reader> && ...)>>
		[[nodiscard]] bool serialize(Args&&... args)
			noexcept((noexcept(std::declval<Args&>().serialize(std::declval<bit_reader&>())) && ...))
		{
			return (std::forward<Args>(args).serialize(*this) && ...);
		}

		/**
		 * @brief Reads from the buffer, using the given @p Trait.
		 * @note The Trait type in this function must always be explicitly declared
		 * @tparam Trait A template specialization of serialize_trait<>
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param ...args The arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename Trait, typename... Args, typename = utility::has_serialize_t<Trait, bit_reader, Args...>>
		[[nodiscard]] bool serialize(Args&&... args) noexcept(utility::is_serialize_noexcept_v<Trait, bit_reader, Args...>)
		{
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
		template<typename... Args, typename Trait, typename = utility::has_deduce_serialize_t<Trait, bit_reader, Args...>>
		[[nodiscard]] bool serialize(Trait&& arg, Args&&... args) noexcept(utility::is_deduce_serialize_noexcept_v<Trait, bit_reader, Args...>)
		{
			return serialize_traits<utility::deduce_trait_t<Trait, bit_reader, Args...>>::serialize(*this, std::forward<Trait>(arg), std::forward<Args>(args)...);
		}

	private:
		Policy m_Policy;

		uint64_t m_Scratch;
		uint32_t m_ScratchBits;
		uint32_t m_WordIndex;
	};

	using fixed_bit_reader = bit_reader<fixed_policy>;
}