#pragma once
#include "Core/Utility.h"
#include "Quantization/BoundedRange.h"
#include "Quantization/SmallestThree.h"
#include <cstdint>
#include <string>

namespace BSNet
{
	/// <summary>
	/// A writer for packets which will be sent using the BSSocket class
	/// </summary>
	class BSWriter
	{
	public:
		enum { writing = 1 };
		enum { reading = 0 };

		uint32_t* buffer;
		uint32_t num_bits_written;

		BSWriter();
		BSWriter(void* bytes);

		/// <summary>
		/// Flushes the current word if any is present.
		/// <para/>Use once you're done writing
		/// </summary>
		/// <returns>The amount of bytes that were written into the buffer</returns>
		uint16_t flush();

		/// <summary>
		/// Appends a checksum at the end of the buffer and flushes the buffer.
		/// <para/>Use once you're done writing
		/// </summary>
		/// <returns>The amount of bytes that were written into the buffer</returns>
		uint16_t serialize_checksum(uint8_t* protocol_version, uint32_t protocol_size);

		/// <summary>
		/// Pads the buffer with zeros up until the given size
		/// </summary>
		/// <param name="size">The amount to pad to, in bytes</param>
		/// <returns>false if the buffer is already past the padding size</returns>
		bool pad_to_size(uint32_t size);

		/// <summary>
		/// Pads the buffer with zeros until the next byte starts
		/// </summary>
		/// <returns>Whether the alignment was a success or not</returns>
		bool align();

		/// <summary>
		/// Serializes the specified amount if bits into the variable
		/// </summary>
		/// <param name="value">The variable to serialize</param>
		/// <param name="num_bits">The number of bits to serialize</param>
		/// <returns>Whether the serialization was successful or not</returns>
		bool serialize_bits(uint32_t& value, uint32_t num_bits);

		/// <summary>
		/// Serializes the specified amount if bits into the byte array
		/// </summary>
		/// <param name="bytes">The variable to serialize</param>
		/// <param name="num_bits">The number of bits to serialize</param>
		/// <returns>Whether the serialization was successful or not</returns>
		bool serialize_bytes(uint8_t* bytes, uint32_t num_bits);

		/// <summary>
		/// Serializes a string
		/// </summary>
		/// <param name="string">The string to serialize</param>
		/// <param name="size">The maximum size of the string</param>
		/// <returns>Whether the serialization was successful or not</returns>
		bool serialize_string(std::string& string, uint32_t size);

		/// <summary>
		/// Serializes a char array
		/// </summary>
		/// <param name="string">The char array to serialize</param>
		/// <param name="size">The size of the char array</param>
		/// <returns>Whether the serialization was successful or not</returns>
		bool serialize_chars(char* string, uint32_t size);

		template<typename T>
		bool serialize_int(T& value, T min = (std::numeric_limits<T>::min)(), T max = (std::numeric_limits<T>::max)())
		{
			BS_ASSERT_RETURN(min < max);

			if (value < min || value > max)
				return false;

			const int num_bits = Utility::bits_in_range(min, max);

			// If the given range is bigger than a word (32 bits)
			int max_words = (num_bits - 1) / 32 + 1;
			for (int i = 0; i < max_words; i++)
			{
				int bit_count = (std::min)(num_bits - i * 32, 32);
				int shift = i * 32;
				uint32_t unsigned_value = (uint32_t)((value - min) >> shift);
				bool status = serialize_bits(unsigned_value, bit_count);
				if (!status)
					return false;
			}

			return true;
		}

		bool serialize_half(float& value);
		bool serialize_float(float& value);
		bool serialize_float(const Quantization::BoundedRange& range, float& value);

		template<typename T>
		bool serialize_vector2(const Quantization::BoundedRange* ranges, T& value)
		{
			auto quantized_vec = Quantization::BoundedRange::quantize_vector2<T>(value, ranges);

			if (!serialize_bits(quantized_vec.x, ranges[0].bits_required))
				return false;

			return serialize_bits(quantized_vec.y, ranges[1].bits_required);
		}

		template<typename T>
		bool serialize_vector3(const Quantization::BoundedRange* ranges, T& value)
		{
			auto quantized_vec = Quantization::BoundedRange::quantize_vector3<T>(value, ranges);

			if (!serialize_bits(quantized_vec.x, ranges[0].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.y, ranges[1].bits_required))
				return false;

			return serialize_bits(quantized_vec.z, ranges[2].bits_required);
		}

		template<typename T>
		bool serialize_vector4(const Quantization::BoundedRange* ranges, T& value)
		{
			auto quantized_vec = Quantization::BoundedRange::quantize_vector4<T>(value, ranges);

			if (!serialize_bits(quantized_vec.x, ranges[0].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.y, ranges[1].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.z, ranges[2].bits_required))
				return false;

			return serialize_bits(quantized_vec.w, ranges[3].bits_required);
		}

		template<typename T>
		bool serialize_quaternion(T& value, uint32_t bits_per_element = 12)
		{
			auto quantized_quat = Quantization::SmallestThree::quantize<T>(value, bits_per_element);

			if (!serialize_bits(quantized_quat.m, 2))
				return false;

			if (!serialize_bits(quantized_quat.a, bits_per_element))
				return false;

			if (!serialize_bits(quantized_quat.b, bits_per_element))
				return false;

			return serialize_bits(quantized_quat.c, bits_per_element);
		}

		bool serialize_endpoint(IPEndPoint& value);

	private:
		uint64_t scratch;
		uint32_t scratch_bits;
		uint32_t word_index;
	};
}