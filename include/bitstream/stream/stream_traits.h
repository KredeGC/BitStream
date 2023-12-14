#pragma once

#include "byte_buffer.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bitstream
{
	struct fixed_policy
	{
		using void_ptr = std::conditional_t<true, void*, const void*>;
		using buffer_ptr = std::conditional_t<true, uint32_t*, const uint32_t*>;

		/**
		 * @brief Construct a stream pointing to the given byte array with @p num_bytes size
		 * @param bytes The byte array to serialize to/from. Must be 4-byte aligned and the size must be a multiple of 4
		 * @param num_bytes The number of bytes in the array
		*/
		fixed_policy(void_ptr buffer, uint32_t num_bits) noexcept :
			m_Buffer(static_cast<buffer_ptr>(buffer)),
			m_NumBitsSerialized(0),
			m_TotalBits(num_bits) {}

		/**
		 * @brief Construct a stream pointing to the given @p buffer
		 * @param buffer The buffer to serialize to/from
		 * @param num_bits The maximum number of bits that we can read
		*/
		template<size_t Size>
		fixed_policy(byte_buffer<Size>& buffer, uint32_t num_bits) noexcept :
			m_Buffer(reinterpret_cast<buffer_ptr>(buffer.Bytes)),
			m_NumBitsSerialized(0),
			m_TotalBits(num_bits) {}

		/**
		 * @brief Construct a stream pointing to the given @p buffer
		 * @param buffer The buffer to serialize to/from
		*/
		template<size_t Size>
		fixed_policy(byte_buffer<Size>& buffer) noexcept :
			m_Buffer(reinterpret_cast<buffer_ptr>(buffer.Bytes)),
			m_NumBitsSerialized(0),
			m_TotalBits(Size * 8) {}

		buffer_ptr get_buffer() const noexcept { return m_Buffer; }

		// TODO: Transition to size_t
		uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsSerialized; }

		bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_NumBitsSerialized + num_bits <= m_TotalBits; }

		bool extend(uint32_t num_bits)
		{
			bool status = can_serialize_bits(num_bits);
			m_NumBitsSerialized += num_bits;
			return status;
		}

		buffer_ptr m_Buffer;
		uint32_t m_NumBitsSerialized;
		uint32_t m_TotalBits;
	};

	template<typename T>
	struct growing_policy
	{
		bool can_serialize_bits(uint32_t bits_written, uint32_t num_bits) const noexcept { return true; }

		bool extend(uint32_t bits_written, uint32_t num_bits) { return ; }

		T Buffer;
	};
}