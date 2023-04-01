#pragma once
#include "../utility/assert.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>

namespace bitstream
{
	/**
	 * @brief A trait used to serialize a float as-is, without any bound checking or quantization
	*/
	template<>
	struct serialize_traits<float>
	{
		/**
		 * @brief Serializes a whole float into the writer
		 * @param writer The stream to write to
		 * @param value The float to serialize
		 * @return Success
		*/
		static bool serialize(bit_writer& writer, float value) noexcept
		{
			uint32_t tmp;
			std::memcpy(&tmp, &value, sizeof(float));

			BS_ASSERT(writer.serialize_bits(tmp, 32));

			return true;
		}

		/**
		 * @brief Serializes a whole float from the reader
		 * @param reader The stream to read from
		 * @param value The float to serialize to
		 * @return Success
		*/
		static bool serialize(bit_reader& reader, float& value) noexcept
		{
			uint32_t tmp;

			BS_ASSERT(reader.serialize_bits(tmp, 32));

			std::memcpy(&value, &tmp, sizeof(float));

			return true;
		}
	};

	/**
	 * @brief A trait used to serialize a double as-is, without any bound checking or quantization
	*/
	template<>
	struct serialize_traits<double>
	{
		/**
		 * @brief Serializes a whole double into the writer
		 * @param writer The stream to write to
		 * @param value The double to serialize
		 * @return Success
		*/
		static bool serialize(bit_writer& writer, double value) noexcept
		{
			uint32_t tmp[2];
			std::memcpy(tmp, &value, sizeof(double));

			BS_ASSERT(writer.serialize_bits(tmp[0], 32));
			BS_ASSERT(writer.serialize_bits(tmp[1], 32));

			return true;
		}

		/**
		 * @brief Serializes a whole double from the reader
		 * @param reader The stream to read from
		 * @param value The double to serialize to
		 * @return Success
		*/
		static bool serialize(bit_reader& reader, double& value) noexcept
		{
			uint32_t tmp[2];

			BS_ASSERT(reader.serialize_bits(tmp[0], 32));
			BS_ASSERT(reader.serialize_bits(tmp[1], 32));

			std::memcpy(&value, tmp, sizeof(double));

			return true;
		}
	};
}