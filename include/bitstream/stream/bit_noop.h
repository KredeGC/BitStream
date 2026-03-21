#pragma once
#include "../utility/meta.h"

#include "multi.h"
#include "serialize_traits.h"
#include "stream_traits.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace bitstream
{
	/**
	 * @brief A noop stream
	*/
	class bit_noop
	{
	public:
		static constexpr bool writing = true;
		static constexpr bool reading = false;

		/**
		 * @brief Returns the buffer that this reader is currently serializing from
		 * @return The buffer
		*/
		[[nodiscard]] const uint8_t* get_buffer() const noexcept { return nullptr; }

		/**
		 * @brief Returns the number of bits which have been read from the buffer
		 * @return The number of bits which have been read
		*/
		[[nodiscard]] uint32_t get_num_bits_serialized() const noexcept { return 0; }

		/**
		 * @brief Returns the number of bytes which have been read from the buffer
		 * @return The number of bytes which have been read
		*/
		[[nodiscard]] uint32_t get_num_bytes_serialized() const noexcept { return 0; }

		/**
		 * @brief Returns whether the @p num_bits be read from the buffer
		 * @param num_bits The number of bits to test
		 * @return Whether the number of bits can be read from the buffer
		*/
		[[nodiscard]] bool can_serialize_bits(uint32_t num_bits) const noexcept { return false; }

		/**
		 * @brief Returns the number of bits which have not been read yet
		 * @note The same as get_total_bits() - get_num_bits_serialized()
		 * @return The remaining space in the buffer
		*/
		[[nodiscard]] uint32_t get_remaining_bits() const noexcept { return 0; }

		/**
		 * @brief Returns the size of the buffer, in bits
		 * @return The size of the buffer, in bits
		*/
		[[nodiscard]] uint32_t get_total_bits() const noexcept { return 0; }

		/**
		 * @brief Pads the buffer up to the given number of bytes
		 * @param num_bytes The byte number to pad to
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes or if the padded bits are not zeros.
		*/
		[[nodiscard]] bool pad_to_size(uint32_t num_bytes) noexcept { return false; }

		/**
		 * @brief Pads the buffer up with the given number of bytes
		 * @param num_bytes The amount of bytes to pad
		 * @return Returns false if the current size of the buffer is bigger than @p num_bytes or if the padded bits are not zeros.
		*/
		[[nodiscard]] bool pad(uint32_t num_bytes) noexcept { return false; }

		/**
		 * @brief Pads the buffer with up to 8 zeros, so that the next read is byte-aligned
		 * @notes Return false if the padded bits are not zeros
		 * @return Returns false if the padded bits are not zeros
		*/
		[[nodiscard]] bool align() noexcept { return false; }

		/**
		 * @brief Reads the first @p num_bits bits of @p value from the buffer
		 * @param value The value to serialize
		 * @param num_bits The number of bits of the @p value to serialize
		 * @return Returns false if @p num_bits is less than 1 or greater than 32 or if reading the given number of bits would overflow the buffer
		*/
		[[nodiscard]] bool serialize_bits(uint32_t& value, uint32_t num_bits) noexcept { return false; }

		/**
		 * @brief Reads the first @p num_bits bits of the given byte array, 32 bits at a time
		 * @param bytes The bytes to serialize
		 * @param num_bits The number of bits of the @p bytes to serialize
		 * @return Returns false if @p num_bits is less than 1 or if reading the given number of bits would overflow the buffer
		*/
		[[nodiscard]] bool serialize_bytes(uint8_t* bytes, uint32_t num_bits) noexcept { return false; }

		/**
		 * @brief Reads from the buffer into mulitple variables.
		 * @note Pass multi<T>(...) to this in place of multiple calls to the regular serialize functions.
		 * @tparam ...Args The types of the arguments to pass to the serialize function
		 * @param ...args The arguments to pass to the serialize function
		 * @return Whether successful or not
		*/
		template<typename... Args, typename = std::enable_if_t<(utility::has_instance_serialize_v<Args, bit_noop> && ...)>>
		[[nodiscard]] bool serialize(Args&&... args)
			noexcept((noexcept(std::declval<Args&>().serialize(std::declval<bit_noop&>())) && ...))
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
		template<typename Trait, typename... Args, typename = utility::has_serialize_t<Trait, bit_noop, Args...>>
		[[nodiscard]] bool serialize(Args&&... args) noexcept(utility::is_serialize_noexcept_v<Trait, bit_noop, Args...>)
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
		template<typename... Args, typename Trait, typename = utility::has_deduce_serialize_t<Trait, bit_noop, Args...>>
		[[nodiscard]] bool serialize(Trait&& arg, Args&&... args) noexcept(utility::is_deduce_serialize_noexcept_v<Trait, bit_noop, Args...>)
		{
			return serialize_traits<utility::deduce_trait_t<Trait, bit_noop, Args...>>::serialize(*this, std::forward<Trait>(arg), std::forward<Args>(args)...);
		}
	};
}