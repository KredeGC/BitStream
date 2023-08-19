#pragma once
#include "../utility/assert.h"
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "../stream/serialize_traits.h"

#include <cstdint>
#include <cstring>

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
		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer, in<float> value) noexcept
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
		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader, float& value) noexcept
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
		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer, in<double> value) noexcept
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
		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader, double& value) noexcept
		{
			uint32_t tmp[2];

			BS_ASSERT(reader.serialize_bits(tmp[0], 32));
			BS_ASSERT(reader.serialize_bits(tmp[1], 32));

			std::memcpy(&value, tmp, sizeof(double));

			return true;
		}
	};
}