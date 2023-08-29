#pragma once

#include "assert.h"

#include <utility>
#include <type_traits>

#ifdef __cpp_constexpr_dynamic_alloc
#define BS_CONSTEXPR constexpr
#else // __cpp_constexpr_dynamic_alloc
#define BS_CONSTEXPR
#endif // __cpp_constexpr_dynamic_alloc

namespace bitstream
{
#ifdef BS_DEBUG_BREAK
	template<typename T>
	class out
	{
	public:
		BS_CONSTEXPR out(T& value) noexcept :
			m_Value(value),
			m_Constructed(false) {}

		out(const out&) = delete;

		out(out&&) = delete;

		BS_CONSTEXPR ~out()
		{
			if (!m_Constructed)
				BS_BREAKPOINT();
		}

		template<typename U, typename = std::enable_if_t<std::is_assignable_v<T&, U>>>
		BS_CONSTEXPR out& operator=(U&& arg) noexcept(std::is_nothrow_assignable_v<T&, U>)
		{
			m_Value = std::forward<U>(arg);

			m_Constructed = true;

			return *this;
		}

		BS_CONSTEXPR T* operator->() noexcept
		{
			m_Constructed = true;
			return &m_Value;
		}

		BS_CONSTEXPR T& operator*() noexcept
		{
			m_Constructed = true;
			return m_Value;
		}

	private:
		T& m_Value;
		bool m_Constructed;
	};
#else
	template<typename T>
	class out
	{
	public:
		BS_CONSTEXPR out(T& value) noexcept :
			m_Value(value) {}

		out(const out&) = delete;

		out(out&&) = delete;

		template<typename U, typename = std::enable_if_t<std::is_assignable_v<T&, U>>>
		BS_CONSTEXPR out& operator=(U&& arg) noexcept(std::is_nothrow_assignable_v<T&, U>)
		{
			m_Value = std::forward<U>(arg);

			return *this;
		}

		BS_CONSTEXPR T* operator->() noexcept { return &m_Value; }

		BS_CONSTEXPR T& operator*() noexcept { return m_Value; }

	private:
		T& m_Value;
	};
#endif

	/**
	 * @brief Passes by const or const reference depending on size
	*/
	template<typename T>
	using in = std::conditional_t<(sizeof(T) <= 16 && std::is_trivially_copy_constructible_v<T>), std::add_const_t<T>, std::add_lvalue_reference_t<std::add_const_t<T>>>;

	/**
	 * @brief Passes by reference
	*/
	template<typename Stream, typename T>
	using inout = std::conditional_t<Stream::writing, in<T>, std::add_lvalue_reference_t<T>>;


	/**
	 * @brief Test type
	*/
	template<typename Lambda>
	class finally
	{
	public:
		constexpr finally(Lambda func) noexcept :
			m_Lambda(func) {}

		~finally()
		{
			m_Lambda();
		}

	private:
		Lambda m_Lambda;
	};

	template<typename Lambda>
	finally(Lambda func) -> finally<Lambda>;
}