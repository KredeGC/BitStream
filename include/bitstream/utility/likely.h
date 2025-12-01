#pragma once

#if !defined(BS_EQUAL_PATH) && defined (__has_cpp_attribute) && __has_cpp_attribute(likely) >= 201803L
#   define BS_LIKELY [[likely]]
#   define BS_UNLIKELY [[unlikely]]
#else
#   define BS_LIKELY
#   define BS_UNLIKELY
#endif // __has_cpp_attribute(likely)