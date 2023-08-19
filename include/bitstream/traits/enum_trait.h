#pragma once
#include "../utility/assert.h"
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "../stream/serialize_traits.h"

#include "../traits/integral_traits.h"

namespace bitstream
{
	/**
	 * @brief Wrapper type for compiletime known integer bounds
	 * @tparam T
	*/
	template<typename T, std::underlying_type_t<T> = (std::numeric_limits<T>::min)(), std::underlying_type_t<T> = (std::numeric_limits<T>::max)()>
	struct bounded_enum;

	/**
	 * @brief A trait used to serialize an enum type with runtime bounds
	*/
	template<typename T>
	struct serialize_traits<T, typename std::enable_if_t<std::is_enum_v<T>>>
	{
        using value_type = std::underlying_type_t<T>;

		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer, T value, value_type min = 0, value_type max = (std::numeric_limits<value_type>::max)()) noexcept
		{
			value_type unsigned_value = static_cast<value_type>(value);

			return writer.template serialize<value_type>(unsigned_value, min, max);
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader, T& value, value_type min = 0, value_type max = (std::numeric_limits<value_type>::max)()) noexcept
		{
			value_type unsigned_value;

			BS_ASSERT(reader.template serialize<value_type>(unsigned_value, min, max));

			value = static_cast<T>(unsigned_value);

			return true;
		}
	};

	/**
	 * @brief A trait used to serialize an enum type with compiletime bounds
	*/
	template<typename T, std::underlying_type_t<T> Min, std::underlying_type_t<T> Max>
	struct serialize_traits<bounded_enum<T, Min, Max>, typename std::enable_if_t<std::is_enum_v<T>>>
	{
		using value_type = std::underlying_type_t<T>;
		using bound_type = bounded_int<value_type, Min, Max>;

		template<typename Stream>
		typename utility::is_writing_t<Stream>
		static serialize(Stream& writer, T value) noexcept
		{
			value_type unsigned_value = static_cast<value_type>(value);

			return writer.template serialize<bound_type>(unsigned_value);
		}

		template<typename Stream>
		typename utility::is_reading_t<Stream>
		static serialize(Stream& reader, T& value) noexcept
		{
			value_type unsigned_value;

			BS_ASSERT(reader.template serialize<bound_type>(unsigned_value));

			value = static_cast<T>(unsigned_value);

			return true;
		}
	};
}