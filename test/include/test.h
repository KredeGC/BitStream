#pragma once

#include <cstddef>
#include <string>

#define BS_ADD_TEST3(func_ptr, line) void func_ptr(); \
	static int test_##line = (::bitstream::test::unit::add_test(#func_ptr, func_ptr), 0); \
	void func_ptr()

#define BS_ADD_TEST2(func_ptr, line) BS_ADD_TEST3(func_ptr, line)

#define BS_ADD_TEST(func_ptr) BS_ADD_TEST2(func_ptr, __LINE__)

namespace bitstream::test
{
	class unit
	{
	private:
		inline constexpr static size_t MAX_TESTS = 1024;

		inline static void (*s_TestFunctions[MAX_TESTS])();
		inline static std::string s_TestNames[MAX_TESTS];
		inline static size_t s_TestCounter;

	public:
		static void add_test(const std::string& name, void (*func_ptr)());

		static void run_all_tests();
	};
}