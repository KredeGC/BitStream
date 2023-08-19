#pragma once
#include "../utility/assert.h"
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "../stream/serialize_traits.h"

namespace bitstream
{
	/**
	 * @brief A trait used to serialize a boolean as a single bit
	*/
	template<>
	struct serialize_traits<bool>
	{
		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer, in<bool> value) noexcept
		{
			uint32_t unsigned_value = value;

			return writer.serialize_bits(unsigned_value, 1U);
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader, out<bool> value) noexcept
		{
			uint32_t unsigned_value;

			BS_ASSERT(reader.serialize_bits(unsigned_value, 1U));

			value = unsigned_value;

			return true;
		}
	};

	/**
	 * @brief A trait used to serialize multiple boolean values
	*/
	template<size_t Size>
	struct serialize_traits<bool[Size]>
	{
		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer, const bool* values) noexcept
		{
			uint32_t unsigned_value;
			for (size_t i = 0; i < Size; i++)
			{
				unsigned_value = values[i];
				BS_ASSERT(writer.serialize_bits(unsigned_value, 1U));
			}

			return writer.serialize_bits(unsigned_value, 1U);
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader, bool* values) noexcept
		{
			uint32_t unsigned_value;
			for (size_t i = 0; i < Size; i++)
			{
				BS_ASSERT(reader.serialize_bits(unsigned_value, 1U));

				values[i] = unsigned_value;
			}

			return true;
		}
	};
}