#pragma once
#include "Core/IPEndPoint.h"
#include "Core/Utility.h"
#include "Quantization/BoundedRange.h"
#include "Quantization/SmallestThree.h"
#include <cstdint>
#include <string>

namespace BSNet
{
	/// <summary>
	/// A reader for reading packets which were written using BSWriter
	/// </summary>
	class BSReader
	{
	public:
		enum { writing = 0 };
		enum { reading = 1 };

		uint32_t* buffer;
		uint32_t num_bits_read;

		BSReader();
		BSReader(void* bytes, uint32_t num_bytes);

		/// <summary>
		/// Whether there is enough space left in the buffer to read the given amount of bits
		/// </summary>
		/// <param name="num_bits">The amount of bits</param>
		/// <returns>Whether the buffer has enough space to read this many bits</returns>
		bool can_read_bits(uint32_t num_bits);

		/// <summary>
		/// Returns the amount of remaining bits (buffer_size - read_bits)
		/// </summary>
		/// <returns>The amount of bits left to read</returns>
		uint16_t get_remaining_bits();

		/// <summary>
		/// Verifies the checksum of the entire buffer using the given protocol version
		/// </summary>
		/// <param name="protocol_version">The protocol version to validate against</param>
		/// <param name="protocol_size">The size of the protocol version in bytes</param>
		/// <returns>Whether the checksum was correct according to the protocol version</returns>
		bool serialize_checksum(uint8_t* protocol_version, uint32_t protocol_size);

		/// <summary>
		/// Verifies that the buffer only has zeros until it reaches the given size
		/// </summary>
		/// <param name="size">The amount to pad to, in bytes</param>
		/// <returns>Whether the padding only contained zeros or not</returns>
		bool pad_to_size(uint32_t size);

		/// <summary>
		/// Verifies that the buffer has been padded until it was byte-aligned
		/// </summary>
		/// <returns>Whether the padding only contained zeros or not</returns>
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

			const int num_bits = Utility::bits_in_range(min, max);

			if (!can_read_bits(num_bits))
				return false;

			value = 0;
			int max_words = (num_bits - 1) / 32 + 1;
			for (int i = 0; i < max_words; i++)
			{
				uint32_t unsigned_value;
				int bit_count = (std::min)(num_bits - i * 32, 32);
				bool status = serialize_bits(unsigned_value, bit_count);
				if (!status)
					return false;

				int shift = i * 32;
				value |= ((T)unsigned_value + min) << shift;
			}

			if (value < min || value > max)
				return false;

			return true;
		}

		bool serialize_half(float& value);
		bool serialize_float(float& value);
		bool serialize_float(const Quantization::BoundedRange& range, float& value);

		template<typename T>
		bool serialize_vector2(const Quantization::BoundedRange* ranges, T& value)
		{
			if (!can_read_bits(ranges[0].bits_required + ranges[1].bits_required))
				return false;

			Quantization::QuantizedVector2 quantized_vec;

			if (!serialize_bits(quantized_vec.x, ranges[0].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.y, ranges[1].bits_required))
				return false;

			value = Quantization::BoundedRange::dequantize_vector2<T>(quantized_vec, ranges);

			return true;
		}

		template<typename T>
		bool serialize_vector3(const Quantization::BoundedRange* ranges, T& value)
		{
			if (!can_read_bits(ranges[0].bits_required + ranges[1].bits_required + ranges[2].bits_required))
				return false;

			Quantization::QuantizedVector3 quantized_vec;

			if (!serialize_bits(quantized_vec.x, ranges[0].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.y, ranges[1].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.z, ranges[2].bits_required))
				return false;

			value = Quantization::BoundedRange::dequantize_vector3<T>(quantized_vec, ranges);

			return true;
		}

		template<typename T>
		bool serialize_vector4(const Quantization::BoundedRange* ranges, T& value)
		{
			if (!can_read_bits(ranges[0].bits_required + ranges[1].bits_required + ranges[2].bits_required + ranges[3].bits_required))
				return false;

			Quantization::QuantizedVector4 quantized_vec;

			if (!serialize_bits(quantized_vec.x, ranges[0].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.y, ranges[1].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.z, ranges[2].bits_required))
				return false;

			if (!serialize_bits(quantized_vec.w, ranges[3].bits_required))
				return false;

			value = Quantization::BoundedRange::dequantize_vector4<T>(quantized_vec, ranges);

			return true;
		}

		template<typename T>
		bool serialize_quaternion(T& value, uint32_t bits_per_element = 12)
		{
			if (!can_read_bits(2 + 3 * bits_per_element))
				return false;

			Quantization::QuantizedQuaternion quantized_quat;

			if (!serialize_bits(quantized_quat.m, 2))
				return false;

			if (!serialize_bits(quantized_quat.a, bits_per_element))
				return false;

			if (!serialize_bits(quantized_quat.b, bits_per_element))
				return false;

			if (!serialize_bits(quantized_quat.c, bits_per_element))
				return false;

			value = Quantization::SmallestThree::dequantize<T>(quantized_quat, bits_per_element);

			return true;
		}

		bool serialize_endpoint(IPEndPoint& value);

	private:
		uint64_t scratch;
		uint32_t scratch_bits;
		uint32_t word_index;
		uint32_t total_bits;
	};
}