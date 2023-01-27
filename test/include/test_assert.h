#pragma once

#include <string>

#define BS_TEST_ASSERT(x) if (!(x)) { throw std::string("\"") + #x + "\" in " + std::string(__FILE__) + ":" + std::to_string(__LINE__); }

#define BS_TEST_ASSERT_FALSE() throw std::string("in ") + std::string(__FILE__) + ":" + std::to_string(__LINE__);