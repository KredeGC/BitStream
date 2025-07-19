#pragma once

#include <type_traits>

#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
#	define BS_CONST_EVALUATED() (std::is_constant_evaluated())
#	define BS_CONSTEXPR constexpr
#else // __cpp_lib_is_constant_evaluated
#	define BS_CONST_EVALUATED() constexpr (false)
#	define BS_CONSTEXPR
#endif // __cpp_lib_is_constant_evaluated