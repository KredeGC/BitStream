#pragma once

#include "../stream/serialize_traits.h"

#include <type_traits>

namespace bitstream::utility
{
	// Check if instance has a serializable trait
	template<typename T, typename Stream, typename Void = void>
	struct has_instance_serialize : std::false_type {};

	template<typename T, typename Stream>
	struct has_instance_serialize<T, Stream, std::void_t<decltype(std::declval<T&>().serialize(std::declval<Stream&>()))>> : std::true_type {};

	template<typename T, typename Stream>
	constexpr bool has_instance_serialize_v = has_instance_serialize<T, Stream>::value;


	// Check if type has a serializable trait
	template<typename Void, typename T, typename Stream, typename... Args>
	struct has_serialize : std::false_type {};

	template<typename T, typename Stream, typename... Args>
	struct has_serialize<std::void_t<decltype(serialize_traits<T>::serialize(std::declval<Stream&>(), std::declval<Args>()...))>, T, Stream, Args...> : std::true_type {};

	template<typename T, typename Stream, typename... Args>
	using has_serialize_t = std::void_t<decltype(serialize_traits<T>::serialize(std::declval<Stream&>(), std::declval<Args>()...))>;

	template<typename T, typename Stream, typename... Args>
	constexpr bool has_serialize_v = has_serialize<void, T, Stream, Args...>::value;


	// Check if stream is writing or reading
	template<typename T, typename R = bool>
	using is_writing_t = std::enable_if_t<T::writing, R>;

	template<typename T, typename R = bool>
	using is_reading_t = std::enable_if_t<T::reading, R>;


	// Check if type is noexcept, if it exists
	template<typename Void, typename T, typename Stream, typename... Args>
	struct is_serialize_noexcept : std::false_type {};

	template<typename T, typename Stream, typename... Args>
	struct is_serialize_noexcept<std::enable_if_t<has_serialize_v<T, Stream, Args...>>, T, Stream, Args...> :
		std::bool_constant<noexcept(serialize_traits<T>::serialize(std::declval<Stream&>(), std::declval<Args>()...))> {};

	template<typename T, typename Stream, typename... Args>
	constexpr bool is_serialize_noexcept_v = is_serialize_noexcept<void, T, Stream, Args...>::value;


	// Get the underlying type without &, &&, * or const
	template<typename T>
	using base_t = typename std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>;


	// Meta functions for guessing the trait type from the first argument
	template<typename Void, typename Trait, typename Stream, typename... Args>
	struct deduce_trait
	{
		using type = Trait;
	};

	// Non-const value
	template<typename Trait, typename Stream, typename... Args>
	struct deduce_trait<std::enable_if_t<
		!std::is_pointer_v<std::decay_t<Trait>> &&
		has_serialize_v<base_t<Trait>, Stream, Trait, Args...>>,
		Trait, Stream, Args...>
	{
		using type = base_t<Trait>;
	};

	// Const value
	template<typename Trait, typename Stream, typename... Args>
	struct deduce_trait<std::enable_if_t<
		!std::is_pointer_v<std::decay_t<Trait>> &&
		has_serialize_v<std::add_const_t<base_t<Trait>>, Stream, Trait, Args...>>,
		Trait, Stream, Args...>
	{
		using type = std::add_const_t<base_t<Trait>>;
	};

	// Non-const pointer
	template<typename Trait, typename Stream, typename... Args>
	struct deduce_trait<std::enable_if_t<
		std::is_pointer_v<std::decay_t<Trait>> &&
		has_serialize_v<std::add_pointer_t<base_t<Trait>>, Stream, Trait, Args...>>,
		Trait, Stream, Args...>
	{
		using type = std::add_pointer_t<base_t<Trait>>;
	};

	// Const pointer
	template<typename Trait, typename Stream, typename... Args>
	struct deduce_trait<std::enable_if_t<
		std::is_pointer_v<std::decay_t<Trait>> &&
		has_serialize_v<std::add_pointer_t<std::add_const_t<base_t<Trait>>>, Stream, Trait, Args...>>,
		Trait, Stream, Args...>
	{
		using type = std::add_pointer_t<std::add_const_t<base_t<Trait>>>;
	};

	template<typename Trait, typename Stream, typename... Args>
	using deduce_trait_t = typename deduce_trait<void, Trait, Stream, Args...>::type;


	// Shorthands for deduced type_traits
	template<typename Trait, typename Stream, typename... Args>
	using has_deduce_serialize_t = has_serialize_t<deduce_trait_t<Trait, Stream, Args...>, Stream, Trait, Args...>;

	template<typename Trait, typename Stream, typename... Args>
	constexpr bool is_deduce_serialize_noexcept_v = is_serialize_noexcept_v<deduce_trait_t<Trait, Stream, Args...>, Stream, Trait, Args...>;
}