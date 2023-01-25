#pragma once
#include "../quantization/bounded_range.h"
#include "../quantization/smallest_three.h"
#include "../utility/assert.h"
#include "../utility/crc.h"
#include "../utility/endian.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

namespace bitstream::stream
{
	class bit_reader
	{
	public:
		static constexpr bool writing = false;
		static constexpr bool reading = true;

		bit_reader() :
			m_Buffer(nullptr),
			m_NumBitsRead(0),
			m_TotalBits(0),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		bit_reader(const void* bytes, uint32_t num_bytes) :
			m_Buffer(static_cast<const uint32_t*>(bytes)),
			m_NumBitsRead(0),
			m_TotalBits(num_bytes * 8),
			m_Scratch(0),
			m_ScratchBits(0),
			m_WordIndex(0) {}

		uint32_t get_num_bits_read() { return m_NumBitsRead; }

		bool can_read_bits(uint32_t num_bits) { return m_NumBitsRead + num_bits <= m_TotalBits; }

		uint32_t get_remaining_bits() { return m_TotalBits - m_NumBitsRead; }

		bool serialize_checksum(uint32_t protocol_version)
		{
			uint32_t num_bytes = (m_TotalBits - 1) / 8 + 1;

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
			m_NumBitsRead += 32;

			// Compare the checksum
			return generated_checksum == checksum;
		}

		bool pad_to_size(uint32_t size)
		{
			if (size * 8 > m_TotalBits || size * 8 < m_NumBitsRead)
				return false;

			// Align with word size
			const int remainder = m_NumBitsRead % 32;
			if (remainder != 0)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 32 - remainder);

				if (!status || zero != 0)
					return false;
			}

			// Test for zeros in padding
			for (uint32_t i = m_WordIndex; i < size / 4; i++)
			{
				uint32_t zero = 0;
				bool status = serialize_bits(zero, 32);

				if (!status || zero != 0)
					return false;
			}

			return true;
		}

		bool align()
		{
			uint32_t remainder = m_NumBitsRead % 8;
			if (remainder != 0)
			{
				uint32_t zero;
				bool status = serialize_bits(zero, 8 - remainder);

				return status && zero == 0 && m_NumBitsRead % 8 == 0;
			}
			return true;
		}

		bool serialize_bits(uint32_t& value, uint32_t num_bits)
		{
			BS_ASSERT(num_bits > 0 && num_bits <= 32);

			BS_ASSERT(can_read_bits(num_bits));

			if (m_ScratchBits < num_bits)
			{
				const uint32_t* ptr = m_Buffer + m_WordIndex;

				uint64_t ptr_value = static_cast<uint64_t>(utility::endian_swap_32(*ptr)) << (32 - m_ScratchBits);
				m_Scratch |= ptr_value;
				m_ScratchBits += 32;
				m_WordIndex++;
			}

			uint32_t offset = 64 - num_bits;
			value = static_cast<uint32_t>(m_Scratch >> offset);

			m_Scratch <<= num_bits;
			m_ScratchBits -= num_bits;
			m_NumBitsRead += num_bits;

			return true;
		}

		bool serialize_bytes(uint8_t* bytes, uint32_t num_bits)
		{
			if (!align())
				return false;

			uint32_t num_bytes = (num_bits - 1) / 8 + 1;
			for (uint32_t i = 0; i < num_bytes; i++)
			{
				uint32_t value;
				if (!serialize_bits(value, (std::min)(num_bits - i * 8U, 8U)))
					return false;
				bytes[i] = static_cast<uint8_t>(value);
			}

			return true;

			//if (!can_read_bits(num_bytes * 8))
			//	return false;

			//// Serialize the first bits normally
			//int offset = 0;
			//while (m_ScratchBits > 0)
			//{
			//	uint32_t byte_value;
			//	if (!serialize_bits(byte_value, 8))
			//		return false;

			//	bytes[offset++] = static_cast<uint8_t>(byte_value);
			//}

			//// Do a straight memcpy with the middle values
			//int size = num_bytes - offset;
			//int last_size = size % 4;
			//int middle_size = size - last_size;

			//std::memcpy(bytes + offset, m_Buffer + m_WordIndex, middle_size); // TODO: Fix warning

			//m_NumBitsRead += (size - last_size) * 8;
			//m_WordIndex += (size - last_size) / 4;

			//// Serialize the last bits normally
			//for (int i = 0; i < last_size; i++)
			//{
			//	uint32_t byte_value;
			//	if (!serialize_bits(byte_value, 8))
			//		return false;

			//	bytes[num_bytes - last_size + i] = static_cast<uint8_t>(byte_value);
			//}

			//return true;
		}

		template<typename Trait, typename... Args>
		bool serialize(Args&&... args)
		{
			return serialize_traits<Trait>::deserialize(*this, std::forward<Args>(args)...);
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