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
#include <memory>
#include <type_traits>

namespace bitstream
{
	/**
	 * @brief A stream for writing objects tightly into a buffer
	 * @tparam Policy The underlying representation of the buffer
	*/
	template<typename Policy>
	class bit_writer
	{
	public:
		static constexpr bool writing = true;
		static constexpr bool reading = false;

		/**
		 * @brief Construct a writer with the parameters passed to the underlying policy
		 * @param ...args The arguments to pass to the policy
		*/
		template<typename... Ts,
			typename = std::enable_if_t<std::is_constructible_v<Policy, Ts...>>>
		bit_writer(Ts&&... args)
			noexcept(std::is_nothrow_constructible_v<Policy, Ts...>) :
			m_Policy(std::forward<Ts>(args) ...),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		bit_writer(const bit_writer&) = delete;
        
		bit_writer(bit_writer&& other) noexcept :
            m_Policy(std::move(other.m_Policy)),
            m_Scratch(other.m_Scratch),
            m_ScratchBits(other.m_ScratchBits),
            m_WordIndex(other.m_WordIndex)
        {
            other.m_Scratch = 0;
            other.m_ScratchBits = 0;
            other.m_WordIndex = 0;
        }

		bit_writer& operator=(const bit_writer&) = delete;

		bit_writer& operator=(bit_writer&& rhs) noexcept
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
		 * @brief Returns the buffer that this writer is currently serializing into
		 * @return The buffer
		*/
		[[nodiscard]] uint8_t* get_buffer() const noexcept { return reinterpret_cast<uint8_t*>(m_Policy.get_buffer()); }

		/**
		 * @brief Returns the number of bits which have been written to the buffer
		 * @return The number of bits which have been written
		*/
		[[nodiscard]] uint32_t get_num_bits_serialized() const noexcept { return m_Policy.get_num_bits_serialized(); }

		/**
		 * @brief Returns the number of bytes which have been written to the buffer
		 * @return The number of bytes which have been written
		*/
		[[nodiscard]] uint32_t get_num_bytes_serialized() const noexcept { return get_num_bits_serialized() > 0U ? ((get_num_bits_serialized() - 1U) / 8U + 1U) : 0U; }

		/**
		 * @brief Returns whether the @p num_bits can fit in the buffer
		 * @param num_bits The number of bits to test
		 * @return Whether the number of bits can fit in the buffer
		*/
		[[nodiscard]] bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_Policy.can_serialize_bits(num_bits); }

		/**
		 * @brief Returns the number of bits which have not been written yet
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
		 * @brief Flushes any remaining bits into the buffer. Use this when you no longer intend to write anything to the buffer.
		 * @return The number of bytes written to the buffer
		*/
		uint32_t flush() noexcept
		{
			if (m_ScratchBits > 0U)
			{
				uint32_t* ptr = m_Policy.get_buffer() + m_WordIndex;
				uint32_t ptr_value = static_cast<uint32_t>(m_Scratch >> 32U);
				*ptr = utility::to_big_endian32(ptr_value);

				m_Scratch = 0U;
				m_ScratchBits = 0U;
				m_WordIndex++;
			}

			return get_num_bits_serialized();
		}

		/**
		 * @brief Pads the buffer up to the given number of bytes with zeros
		 * @param num_bytes The byte number to pad to
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes
		*/
		[[nodiscard]] bool pad_to_size(uint32_t num_bytes) noexcept
		{
			uint32_t num_bits_written = get_num_bits_serialized();

			BS_ASSERT(num_bytes * 8U >= num_bits_written);

			BS_ASSERT(can_serialize_bits(num_bytes * 8U - num_bits_written));

            if (num_bits_written == 0)
            {
				BS_ASSERT(m_Policy.extend(num_bytes * 8U - num_bits_written));

                std::memset(m_Policy.get_buffer(), 0, num_bytes);
                
                m_Scratch = 0;
                m_ScratchBits = 0;
                m_WordIndex = num_bytes / 4;
                
                return true;
            }
            
            uint32_t remainder = (num_bytes * 8U - num_bits_written) % 32U;
			uint32_t zero = 0;

			// Align to byte
			if (remainder != 0U)
				BS_ASSERT(serialize_bits(zero, remainder));

			uint32_t offset = get_num_bits_serialized() / 32;
            uint32_t max = num_bytes / 4;

			// Serialize words
			for (uint32_t i = offset; i < max; i++)
				BS_ASSERT(serialize_bits(zero, 32));

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

				BS_ASSERT(status && get_num_bits_serialized() % 8U == 0U);
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

			BS_ASSERT(m_Policy.extend(num_bits));

			// This is actually slower
			// Possibly due to unlikely branching
			/*if (num_bits == 32U && m_ScratchBits == 0U)
			{
				uint32_t* ptr = m_Policy.get_buffer() + m_WordIndex;

				*ptr = utility::to_big_endian32(value);

				m_WordIndex++;

				return true;
			}*/

			uint32_t offset = 64U - num_bits - m_ScratchBits;
			uint64_t ls_value = static_cast<uint64_t>(value) << offset;

			m_Scratch |= ls_value;
			m_ScratchBits += num_bits;

			if (m_ScratchBits >= 32U)
			{
				uint32_t* ptr = m_Policy.get_buffer() + m_WordIndex;
				uint32_t ptr_value = static_cast<uint32_t>(m_Scratch >> 32U);
				*ptr = utility::to_big_endian32(ptr_value);
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
				BS_ASSERT(m_Policy.extend(num_words * 32U));

                // If the written buffer is word-aligned, just memcpy it
                std::memcpy(m_Policy.get_buffer() + m_WordIndex, word_buffer, num_words * 4U);
                
                m_WordIndex += num_words;
            }
            else
            {
                // If the buffer is not word-aligned, serialize a word at a time
                for (uint32_t i = 0U; i < num_words; i++)
                {
                    // Casting a byte-array to an int is wrong on little-endian systems
                    // We have to swap the bytes around
                    uint32_t value = utility::to_big_endian32(word_buffer[i]);
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
		template<typename T>
		[[nodiscard]] bool serialize_into(bit_writer<T>& writer) const noexcept
		{
			uint8_t* buffer = reinterpret_cast<uint8_t*>(m_Policy.get_buffer());
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

		template<typename... Args, typename = std::enable_if_t<(utility::has_instance_serialize_v<Args, bit_writer> && ...)>>
		[[nodiscard]] bool serialize(Args&&... args)
			noexcept((noexcept(std::declval<Args&>().serialize(std::declval<bit_writer&>())) && ...))
		{
			return (std::forward<Args>(args).serialize(*this) && ...);
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
		Policy m_Policy;

		uint64_t m_Scratch;
		int m_ScratchBits;
		size_t m_WordIndex;
	};

	using fixed_bit_writer = bit_writer<fixed_policy>;

	template<typename T>
	using growing_bit_writer = bit_writer<growing_policy<T>>;
}