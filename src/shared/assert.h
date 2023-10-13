#pragma once

#include <bitstream/utility/platform.h>

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

namespace bitstream::test
{
    struct test_result
    {
        bool Success;
        size_t Size;
        char Message[256];

        constexpr test_result() noexcept :
            Success(true),
            Size(0),
            Message{ 0 } {}

        constexpr test_result(std::string_view view) noexcept :
            Success(false),
            Size(0),
            Message{ 0 }
        {
            std::copy(view.begin(), view.end(), Message);
        }
    };

    template<typename T, typename... Args>
    inline std::string to_string(const T& value)
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return std::to_string(value);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return value;
        }
        else
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
    }
}

#define BS_TEST_STRINGIZE(x) #x
#define BS_TEST_STRINGIZE2(x) BS_TEST_STRINGIZE(x)
#define BS_TEST_LINE BS_TEST_STRINGIZE2(__LINE__)

#define BS_TEST_ASSERT(x) if (!(x)) { return ::bitstream::test::test_result("\"" #x "\" in " __FILE__ ":" BS_TEST_LINE); }

#define BS_TEST_ASSERT_FALSE() return ::bitstream::test::test_result("in " __FILE__ ":" BS_TEST_LINE);

// Deprecated
#define BS_TEST_ASSERT_OPERATION(x, op, y) if (!(x op y)) { throw std::string("\"") + bitstream::test::to_string(x) + " " + std::string(#op) + " " + bitstream::test::to_string(y) + "\" in " + std::string(__FILE__) + ":" + std::to_string(__LINE__); }