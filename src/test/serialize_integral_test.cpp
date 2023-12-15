#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/integral_traits.h>

namespace bitstream::test::traits
{
	template<typename T, T Min, T Max>
	void test_integral(T value)
	{
		// Test integral numbers
		byte_buffer<8> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<T>(value, Min, Max));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, <= , 8 * sizeof(T));
		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), <= , sizeof(T));


		T out_value = 0;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<T>(out_value, Min, Max));

		BS_TEST_ASSERT_OPERATION(out_value, == , value);
	}

	template<typename T, T Min, T Max>
	void test_const_integral(T value)
	{
		using trait = bounded_int<T, Min, Max>;

		// Test integral numbers
		byte_buffer<8> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<trait>(value));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(num_bits, <= , 8 * sizeof(T));
		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), <= , sizeof(T));


		T out_value = 0;
		bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<trait>(out_value));

		BS_TEST_ASSERT_OPERATION(out_value, == , value);
	}

#pragma region unsigned integral types
	BS_ADD_TEST(test_serialize_uint8)
	{
		test_integral<uint8_t, 20U, 127U>(98);
	}

	BS_ADD_TEST(test_serialize_uint16)
	{
		test_integral<uint16_t, 20U, 400U>(131U);
	}

	BS_ADD_TEST(test_serialize_uint32)
	{
		test_integral<uint32_t, 20U, 92060U>(65001U);
	}

	BS_ADD_TEST(test_serialize_uint64)
	{
		test_integral<uint64_t, 80ULL, 4398046511104ULL>(1099511627776ULL);
	}
#pragma endregion

#pragma region signed integral types
	BS_ADD_TEST(test_serialize_int8)
	{
		test_integral<int8_t, -20, 58>(-11);
	}

	BS_ADD_TEST(test_serialize_int16)
	{
		test_integral<int16_t, -200, 200>(-131);
	}

	BS_ADD_TEST(test_serialize_int32)
	{
		test_integral<int32_t, -92060, 520>(-65001);
	}

	BS_ADD_TEST(test_serialize_int64)
	{
		test_integral<int64_t, -4398046511104LL, 128LL>(-109951127776LL);
	}
#pragma endregion

#pragma region unsigned const integral types
	BS_ADD_TEST(test_serialize_uint8_const)
	{
		test_const_integral<uint8_t, 20U, 127U>(98);
	}

	BS_ADD_TEST(test_serialize_uint16_const)
	{
		test_const_integral<uint16_t, 20U, 400U>(131U);
	}

	BS_ADD_TEST(test_serialize_uint32_const)
	{
		test_const_integral<uint32_t, 20U, 92060U>(65001U);
	}

	BS_ADD_TEST(test_serialize_uint64_const_large)
	{
		test_const_integral<uint64_t, 80ULL, 4398046511104ULL>(1099511627776ULL);
	}

	BS_ADD_TEST(test_serialize_uint64_const_small)
	{
		test_const_integral<uint64_t, 80ULL, 2147483648ULL>(1073741824ULL);
	}
#pragma endregion

#pragma region signed const integral types
	BS_ADD_TEST(test_serialize_int8_const)
	{
		test_const_integral<int8_t, -20, 58>(-11);
	}

	BS_ADD_TEST(test_serialize_int16_const)
	{
		test_const_integral<int16_t, -200, 200>(-131);
	}

	BS_ADD_TEST(test_serialize_int32_const)
	{
		test_const_integral<int32_t, -92060, 520>(-65001);
	}

	BS_ADD_TEST(test_serialize_int64_const_large)
	{
		test_const_integral<int64_t, -4398046511104LL, 128LL>(-109951127776LL);
	}

	BS_ADD_TEST(test_serialize_int64_const_small)
	{
		test_const_integral<int64_t, -2147483648LL, 80LL>(-1073741824LL);
	}
#pragma endregion
}