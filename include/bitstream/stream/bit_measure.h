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
	class bit_measure
	{
	public:
		static constexpr bool writing = true;
		static constexpr bool reading = false;

		/**
		 * @brief Default construct a writer pointing to a null buffer
		*/
		bit_measure() noexcept :
			m_NumBitsWritten(0),
			m_TotalBits(0) {}

		/**
		 * @brief Construct a writer pointing to the given byte array with @p num_bytes size
		 * @param num_bytes The number of bytes in the array
		*/
		explicit bit_measure(uint32_t num_bytes) noexcept :
			m_NumBitsWritten(0),
			m_TotalBits(num_bytes * 8) {}

		bit_measure(const bit_measure&) = delete;

		bit_measure(bit_measure&& other) noexcept :
			m_NumBitsWritten(other.m_NumBitsWritten),
			m_TotalBits(other.m_TotalBits)
		{
			other.m_NumBitsWritten = 0;
			other.m_TotalBits = 0;
		}

		bit_measure& operator=(const bit_measure&) = delete;

		bit_measure& operator=(bit_measure&& rhs) noexcept
		{
			m_NumBitsWritten = rhs.m_NumBitsWritten;
			m_TotalBits = rhs.m_TotalBits;

			rhs.m_NumBitsWritten = 0;
			rhs.m_TotalBits = 0;

			return *this;
		}

		/**
		 * @brief Returns the buffer that this writer is currently serializing into
		 * @return The buffer
		*/
		[[nodiscard]] uint8_t* get_buffer() const noexcept { return nullptr; }

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
		 * @brief Instructs the writer that you intend to use `serialize_checksum()` later on, and to reserve the first 32 bits.
		 * @return Returns false if anything has already been written to the buffer or if there's no space to write the checksum
		*/
		[[nodiscard]] bool prepend_checksum() noexcept
		{
			BS_ASSERT(m_NumBitsWritten == 0);

			BS_ASSERT(can_serialize_bits(32U));

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
			return m_NumBitsWritten;
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

			m_NumBitsWritten = num_bytes * 8U;

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
			uint32_t remainder = m_NumBitsWritten % 8U;
			if (remainder != 0U)
				m_NumBitsWritten += 8U - remainder;
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

			m_NumBitsWritten += num_bits;

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

			m_NumBitsWritten += num_bits;

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
		template<typename Trait, typename... Args, typename = utility::has_serialize_t<Trait, bit_measure, Args...>>
		[[nodiscard]] bool serialize(Args&&... args) noexcept(utility::is_serialize_noexcept_v<Trait, bit_measure, Args...>)
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
		template<typename... Args, typename Trait, typename = utility::has_deduce_serialize_t<Trait, bit_measure, Args...>>
		[[nodiscard]] bool serialize(Trait&& arg, Args&&... args) noexcept(utility::is_deduce_serialize_noexcept_v<Trait, bit_measure, Args...>)
		{
			return serialize_traits<utility::deduce_trait_t<Trait, bit_measure, Args...>>::serialize(*this, std::forward<Trait>(arg), std::forward<Args>(args)...);
		}

	private:
		uint32_t m_NumBitsWritten;
		uint32_t m_TotalBits;
	};
}