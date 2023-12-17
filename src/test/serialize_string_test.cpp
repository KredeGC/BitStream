#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/string_traits.h>

namespace bitstream::test::traits
{
#pragma region const char*
	BS_ADD_TEST(test_serialize_chars_aligned)
	{
		// Test c-style strings
		uint32_t padding = 233;
		const char* value = "Hello, world!";

		// Write a char array, but make sure the word count isn't whole
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 17);

		// Read the array back and validate
		uint32_t out_padding;
		char out_value[32];
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U));

		BS_TEST_ASSERT_OPERATION(out_padding, ==, padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}

	BS_ADD_TEST(test_serialize_chars_misaligned)
	{
		// Test c-style strings
		uint32_t padding = 42;
		const char* value = "Hello, world!";

		// Write a char array, but with an uneven bit offset
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 6));
		BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 15);

		// Read the array back and validate
		uint32_t out_padding;
		char out_value[32];
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 6));
		BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U));

		BS_TEST_ASSERT_OPERATION(out_padding, ==, padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}

	BS_ADD_TEST(test_serialize_bounded_chars_aligned)
	{
		using bounded_type = bounded_string<const char*, 32U>;

		// Test c-style strings
		uint32_t padding = 233;
		const char* value = "Hello, world!";

		// Write a char array, but make sure the word count isn't whole
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<bounded_type>(value));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), == , 17);

		// Read the array back and validate
		uint32_t out_padding;
		char out_value[32];
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<bounded_type>(out_value));

		BS_TEST_ASSERT_OPERATION(out_padding, == , padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}
#pragma endregion

#ifdef __cpp_char8_t
	BS_ADD_TEST(test_serialize_u8_chars_aligned)
	{
		// Test c-style strings
		uint32_t padding = 233;
		const char8_t value[] = u8"Hell\u00FC, world!";

		// Write a char array, but make sure the word count isn't whole
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<const char8_t*>(value, 32U));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 18);

		// Read the array back and validate
		uint32_t out_padding;
		char8_t out_value[32];
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<const char8_t*>(out_value, 32U));

		BS_TEST_ASSERT_OPERATION(out_padding, ==, padding);

		const char8_t* i = value;
		const char8_t* j = out_value;
		while (true)
		{
			BS_TEST_ASSERT_OPERATION(*i, ==, *j);
			if (*i == '\0')
				break;
			++i;
			++j;
		}
	}

	BS_ADD_TEST(test_serialize_u8_string_aligned)
	{
		// Test std strings
		uint32_t padding = 233;
		std::u8string value = u8"Hell\u00FC, world!";

		// Write a string, but make sure the word count isn't whole
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<std::u8string>(value, 32U));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 18);

		// Read the array back and validate
		uint32_t out_padding;
		std::u8string out_value;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<std::u8string>(out_value, 32U));

		BS_TEST_ASSERT_OPERATION(out_padding, ==, padding);
		BS_TEST_ASSERT_OPERATION(out_value.compare(value), ==, 0);
	}
#endif

#pragma region std::basic_string
	BS_ADD_TEST(test_serialize_string_aligned)
	{
		// Test std strings
		uint32_t padding = 233;
		std::string value = "Hello, world!";

		// Write a string, but make sure the word count isn't whole
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<std::string>(value, 32U));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 17);

		// Read the array back and validate
		uint32_t out_padding;
		std::string out_value;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<std::string>(out_value, 32U));

		BS_TEST_ASSERT_OPERATION(out_padding, ==, padding);
		BS_TEST_ASSERT_OPERATION(out_value, ==, value);
	}

	BS_ADD_TEST(test_serialize_wstring_aligned)
	{
		// Test widechar strings
		std::wstring value = L"Hello, world!";

		// Write a widechar string, but make sure the word count isn't whole
		byte_buffer<64> buffer; // wchar_t is apparently platform dependent and can be 16 or 32 bits
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize<std::wstring>(value, 32U));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 1 + 13 * sizeof(wchar_t));

		// Read the array back and validate
		std::wstring out_value;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize<std::wstring>(out_value, 32U));

		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_bounded_string_aligned)
	{
		using bounded_type = bounded_string<std::string, 32U>;

		// Test std strings
		uint32_t padding = 233;
		std::string value = "Hello, world!";

		// Write a string, but make sure the word count isn't whole
		byte_buffer<32> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<bounded_type>(value));
		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT_OPERATION(writer.get_num_bytes_serialized(), ==, 17);

		// Read the array back and validate
		uint32_t out_padding;
		std::string out_value;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<bounded_type>(out_value));

		BS_TEST_ASSERT_OPERATION(out_padding, ==, padding);
		BS_TEST_ASSERT_OPERATION(out_value, ==, value);
	}
#pragma endregion
}