#pragma once

#include "byte_buffer.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace bitstream
{
	struct fixed_policy
	{
		/**
		 * @brief Construct a stream pointing to the given byte array with @p num_bytes size
		 * @param bytes The byte array to serialize to/from. Must be 4-byte aligned and the size must be a multiple of 4
		 * @param num_bytes The number of bytes in the array
		*/
		fixed_policy(void* buffer, uint32_t num_bits) noexcept :
			m_Buffer(static_cast<uint32_t*>(buffer)),
			m_NumBitsSerialized(0),
			m_TotalBits(num_bits) {}

		/**
		 * @brief Construct a stream pointing to the given @p buffer
		 * @param buffer The buffer to serialize to/from
		 * @param num_bits The maximum number of bits that we can read
		*/
		template<size_t Size>
		fixed_policy(byte_buffer<Size>& buffer, uint32_t num_bits) noexcept :
			m_Buffer(reinterpret_cast<uint32_t*>(buffer.Bytes)),
			m_NumBitsSerialized(0),
			m_TotalBits(num_bits) {}

		/**
		 * @brief Construct a stream pointing to the given @p buffer
		 * @param buffer The buffer to serialize to/from
		*/
		template<size_t Size>
		fixed_policy(byte_buffer<Size>& buffer) noexcept :
			m_Buffer(reinterpret_cast<uint32_t*>(buffer.Bytes)),
			m_NumBitsSerialized(0),
			m_TotalBits(Size * 8) {}

		uint32_t* get_buffer() const noexcept { return m_Buffer; }

		// TODO: Transition sizes to size_t
		uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsSerialized; }

		bool can_serialize_bits(uint32_t num_bits) const noexcept { return m_NumBitsSerialized + num_bits <= m_TotalBits; }

		uint32_t get_total_bits() const noexcept { return m_TotalBits; }

		bool extend(uint32_t num_bits)
		{
			bool status = can_serialize_bits(num_bits);
			m_NumBitsSerialized += num_bits;
			return status;
		}

		uint32_t* m_Buffer;
		// TODO: Transition sizes to size_t
		uint32_t m_NumBitsSerialized;
		uint32_t m_TotalBits;
	};

	template<typename T>
	struct growing_policy
	{
		growing_policy(T& container) noexcept :
			m_Buffer(container),
			m_NumBitsSerialized(0) {}

		uint32_t* get_buffer() const noexcept { return m_Buffer.data(); }

		uint32_t get_num_bits_serialized() const noexcept { return m_NumBitsSerialized; }

		bool can_serialize_bits(uint32_t num_bits) const noexcept { return true; }

		uint32_t get_total_bits() const noexcept { return (std::numeric_limits<uint32_t>::max)(); }

		bool extend(uint32_t num_bits)
		{
			m_NumBitsSerialized += num_bits;
			uint32_t num_bytes = (m_NumBitsSerialized - 1) / 8U + 1;
			m_Buffer.resize(num_bytes);
			return true;
		}

		T& m_Buffer;

		uint32_t m_NumBitsSerialized;
	};
}