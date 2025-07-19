#pragma once
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "serialize_traits.h"

#include <tuple>

namespace bitstream
{
	namespace utility
	{
		// Check if type has a serializable trait
		template<typename T, typename Stream, typename Void = void>
		struct has_instance_serialize : std::false_type {};

		template<typename T, typename Stream>
		struct has_instance_serialize<T, Stream, std::void_t<decltype(std::declval<T&>().serialize(std::declval<Stream&>()))>> : std::true_type {};

		template<typename T, typename Stream>
		constexpr bool has_instance_serialize_v = has_instance_serialize<T, Stream>::value;
	}


	template<typename T, typename... Ts>
	struct multi_tuple
	{
		std::tuple<Ts...> Args;

		template<typename Stream, typename = utility::has_serialize_t<T, Stream, Ts...>>
		bool serialize(Stream& stream) noexcept(utility::is_serialize_noexcept_v<T, Stream, Ts...>)
		{
			return std::apply([&](auto&&... args) { return serialize_traits<T>::serialize(stream, args ...); }, std::move(Args));
		}
	};

	template<typename T, typename... Args>
	multi_tuple<T, Args&&...> multi(Args&&... args) noexcept
	{
		return multi_tuple<T, Args&&...>{ std::forward_as_tuple(std::forward<Args>(args) ...) };
	}
}