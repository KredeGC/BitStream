#pragma once

#include "assert.h"

#include <cstddef>
#include <string>

#define BS_ADD_TEST3(func_ptr, line) ::bitstream::test::test_result func_ptr(); \
	struct test_struct_##func_ptr##_##line { test_struct_##func_ptr##_##line() { ::bitstream::test::unit::add_test(#func_ptr, func_ptr); } }; \
	static test_struct_##func_ptr##_##line test_var_##func_ptr##_##line; \
	::bitstream::test::test_result func_ptr()

#define BS_ADD_TEST2(func_ptr, line) BS_ADD_TEST3(func_ptr, line)

#define BS_ADD_TEST(func_ptr) BS_ADD_TEST2(func_ptr, __LINE__)


#define BS_ADD_CONST_TEST3(func_ptr, line, ...) consteval ::bitstream::test::test_result func_ptr##_impl() { __VA_ARGS__ return {}; } \
	::bitstream::test::test_result func_ptr() { return func_ptr##_impl(); } \
	struct test_struct_##func_ptr##_##line { test_struct_##func_ptr##_##line() { ::bitstream::test::unit::add_test(#func_ptr, func_ptr); } }; \
	static test_struct_##func_ptr##_##line test_var_##func_ptr##_##line;

#define BS_ADD_CONST_TEST2(func_ptr, line, ...) BS_ADD_CONST_TEST3(func_ptr, line, __VA_ARGS__)

#define BS_ADD_CONST_TEST(func_ptr, ...) BS_ADD_CONST_TEST2(func_ptr, __LINE__, __VA_ARGS__)

namespace bitstream::test
{
	class unit
	{
	private:
		inline constexpr static size_t MAX_TESTS = 1024;
        
        typedef test_result(*test_ptr_t)();

		inline static test_ptr_t s_TestFunctions[MAX_TESTS];
		inline static std::string s_TestNames[MAX_TESTS];
		inline static size_t s_TestCounter;

	public:
		static void add_test(const std::string& name, test_ptr_t func_ptr);

		static void run_all_tests();
	};
}