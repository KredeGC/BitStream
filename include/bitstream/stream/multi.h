#pragma once
#include "../utility/meta.h"
#include "../utility/parameter.h"

#include "../stream/bit_noop.h"

#include "serialize_traits.h"

#include <tuple>

namespace bitstream
{
	template<typename T, typename... Ts>
	struct multi_args
	{
		std::tuple<Ts...> Args;

		template<typename Stream, typename = utility::has_serialize_t<T, Stream, Ts...>>
		[[nodiscard]] bool serialize(Stream& stream) noexcept(utility::is_serialize_noexcept_v<T, Stream, Ts...>)
		{
			return std::apply([&](auto&&... args) { return serialize_traits<T>::serialize(stream, args ...); }, std::move(Args));
		}
	};

	template<typename T, typename... Args>
	[[nodiscard]] multi_args<T, Args&&...> multi(Args&&... args) noexcept
	{
		return multi_args<T, Args&&...>{ std::forward_as_tuple(std::forward<Args>(args) ...) };
	}

	template<typename... Args, typename Trait, typename = utility::has_deduce_serialize_t<Trait, bit_noop, Args...>>
	[[nodiscard]] multi_args<utility::deduce_trait_t<Trait, bit_noop, Args...>, Trait&&, Args&&...> multi(Trait&& arg, Args&&... args) noexcept
	{
		return multi_args<utility::deduce_trait_t<Trait, bit_noop, Args...>, Trait&&, Args&&...>{ std::forward_as_tuple(std::forward<Trait>(arg), std::forward<Args>(args) ...) };
	}
}