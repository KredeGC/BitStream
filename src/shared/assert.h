#pragma once

#include <string>
#include <type_traits>

namespace bitstream::test
{
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
        
        return std::string();
    }
}

#define BS_TEST_ASSERT(x) if (!(x)) { throw std::string("\"") + #x + "\" in " + std::string(__FILE__) + ":" + std::to_string(__LINE__); }

#define BS_TEST_ASSERT_OPERATION(x, op, y) if (!(x op y)) { throw std::string("\"") + bitstream::test::to_string(x) + " " + std::string(#op) + " " + bitstream::test::to_string(y) + "\" in " + std::string(__FILE__) + ":" + std::to_string(__LINE__); }

#define BS_TEST_ASSERT_FALSE() throw std::string("in ") + std::string(__FILE__) + ":" + std::to_string(__LINE__);