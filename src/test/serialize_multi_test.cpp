#include "../shared/assert.h"
#include "../shared/test.h"

#include <bitstream/stream/bit_reader.h>
#include <bitstream/stream/bit_writer.h>

#include <bitstream/traits/integral_traits.h>
#include <bitstream/traits/quantization_traits.h>
#include <bitstream/traits/string_traits.h>

namespace bitstream::test::multi_serialize
{
	struct custom_multi_type
	{
		bool enabled;
		int count;
		std::string name;
	};
}

namespace bitstream
{
	template<>
	struct serialize_traits<bitstream::test::multi_serialize::custom_multi_type>
	{
		template<typename Stream>
		static bool serialize(Stream& stream, inout<Stream, bitstream::test::multi_serialize::custom_multi_type> value) noexcept
		{
			return stream.serialize(
				multi(value.enabled),
				multi(value.count),
				multi(value.name, 32U)
			);
		}
	};
}

namespace bitstream::test::multi_serialize
{
	BS_ADD_TEST(test_serialize_multi)
	{
		// Test serializing multiple values at once
		uint32_t in_value1 = 511;
		float in_value2 = 99.12345f;

		bounded_range range(-1000.0f, 1000.0f, 0.001f);

		// Write some values
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize(
			multi<uint32_t>(in_value1, 328, 611),
			multi<bounded_range>(range, in_value2)
		));

		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT(num_bits == 30);

		// Read the values back and validate
		uint32_t out_value1;
		float out_value2;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize(
			multi<uint32_t>(out_value1, 328U, 611U),
			multi<bounded_range>(range, out_value2)
		));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(std::abs(in_value2 - out_value2) <= range.get_precision());
	}

	BS_ADD_TEST(test_serialize_multi_deduce)
	{
		// Test serializing multiple values at once
		uint32_t in_value1 = 511;
		float in_value2 = 99.12345f;

		bounded_range range(-1000.0f, 1000.0f, 0.001f);

		// Write some values
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize(
			multi(in_value1, 328, 611),
			multi(range, in_value2)
		));

		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT(num_bits == 30);

		// Read the values back and validate
		uint32_t out_value1;
		float out_value2;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize(
			multi(out_value1, 328U, 611U),
			multi(range, out_value2)
		));

		BS_TEST_ASSERT(out_value1 == in_value1);
		BS_TEST_ASSERT(std::abs(in_value2 - out_value2) <= range.get_precision());
	}

	BS_ADD_TEST(test_serialize_multi_type)
	{
		// Test serializing multiple values at once
		custom_multi_type in_value
		{
			true,
			43,
			"Todd"
		};

		bounded_range range(-1000.0f, 1000.0f, 0.001f);

		// Write some values
		byte_buffer<16> buffer;
		fixed_bit_writer writer(buffer);

		BS_TEST_ASSERT(writer.serialize(in_value));

		uint32_t num_bits = writer.flush();

		BS_TEST_ASSERT(num_bits == 71);

		// Read the values back and validate
		custom_multi_type out_value;
		fixed_bit_reader reader(buffer, num_bits);

		BS_TEST_ASSERT(reader.serialize(out_value));

		BS_TEST_ASSERT(out_value.enabled == in_value.enabled);
		BS_TEST_ASSERT(out_value.count == in_value.count);
		BS_TEST_ASSERT(out_value.name == in_value.name);
	}
}