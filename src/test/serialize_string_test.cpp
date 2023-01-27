#include "../assert.h"
#include "../test.h"

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
		uint8_t buffer[32];
		bit_writer writer(buffer, 32);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 17);

		// Read the array back and validate
		uint32_t out_padding;
		char out_value[32];
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U));

		BS_TEST_ASSERT(out_padding == padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}

	BS_ADD_TEST(test_serialize_chars_misaligned)
	{
		// Test c-style strings
		uint32_t padding = 42;
		const char* value = "Hello, world!";

		// Write a char array, but with an uneven bit offset
		uint8_t buffer[32];
		bit_writer writer(buffer, 32);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 6));
		BS_TEST_ASSERT(writer.serialize<const char*>(value, 32U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 15);

		// Read the array back and validate
		uint32_t out_padding;
		char out_value[32];
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 6));
		BS_TEST_ASSERT(reader.serialize<const char*>(out_value, 32U));

		BS_TEST_ASSERT(out_padding == padding);
		BS_TEST_ASSERT(strcmp(out_value, value) == 0);
	}
#pragma endregion

#pragma region std::basic_string
	BS_ADD_TEST(test_serialize_string_aligned)
	{
		// Test std strings
		uint32_t padding = 233;
		std::string value = "Hello, world!";

		// Write a string, but make sure the word count isn't whole
		uint8_t buffer[32];
		bit_writer writer(buffer, 32);

		BS_TEST_ASSERT(writer.serialize_bits(padding, 26));
		BS_TEST_ASSERT(writer.serialize<std::string>(value, 32U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 17);

		// Read the array back and validate
		uint32_t out_padding;
		std::string out_value;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize_bits(out_padding, 26));
		BS_TEST_ASSERT(reader.serialize<std::string>(out_value, 32U));

		BS_TEST_ASSERT(out_padding == padding);
		BS_TEST_ASSERT(out_value == value);
	}

	BS_ADD_TEST(test_serialize_wstring_aligned)
	{
		// Test widechar strings
		std::wstring value = L"Hello, world!";

		// Write a widechar string, but make sure the word count isn't whole
		uint8_t buffer[64];
		bit_writer writer(buffer, 64); // wchar_t is apparently platform dependent and can be 16 or 32 bits

		BS_TEST_ASSERT(writer.serialize<std::wstring>(value, 32U));
		uint32_t num_bytes = writer.flush();

		BS_TEST_ASSERT(num_bytes == 1 + 13 * sizeof(wchar_t));

		// Read the array back and validate
		std::wstring out_value;
		bit_reader reader(buffer, num_bytes);

		BS_TEST_ASSERT(reader.serialize<std::wstring>(out_value, 32U));

		BS_TEST_ASSERT(out_value == value);
	}
#pragma endregion
}