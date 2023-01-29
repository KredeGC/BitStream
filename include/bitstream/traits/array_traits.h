#pragma once
#include "../utility/bits.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include <cstdint>

namespace bitstream
{
	template<typename T, typename = T>
	struct array_subset;

	template<typename T, typename Trait>
	struct serialize_traits<array_subset<T, Trait>>
	{
		template<typename Compare, typename... Args>
		static bool serialize(bit_writer& writer, T* values, uint32_t max_size, Compare compare, Args&&... args) noexcept
		{
			uint32_t prev_index = 0;
			for (uint32_t i = 0; i < max_size; i++)
			{
				if (!compare(values[i]))
					continue;

				uint32_t num_bits = utility::bits_in_range(prev_index, max_size);

				uint32_t index = i - prev_index;
				BS_ASSERT(writer.serialize_bits(index, num_bits));

				BS_ASSERT(writer.serialize<Trait>(values[i], std::forward<Args>(args)...));

				prev_index = i;
			}

			uint32_t num_bits = utility::bits_in_range(prev_index, max_size);

			BS_ASSERT(writer.serialize_bits(max_size - prev_index, num_bits));

			return true;
		}

		template<typename Compare, typename... Args>
		static bool serialize(bit_reader& reader, T* values, uint32_t max_size, Compare compare, Args&&... args) noexcept
		{
			uint32_t prev_index = 0;
			for (uint32_t i = 0; i <= max_size; i++)
			{
				uint32_t num_bits = utility::bits_in_range(prev_index, max_size);

				uint32_t index;
				BS_ASSERT(reader.serialize_bits(index, num_bits));

				if (index + prev_index == max_size)
					break;

				BS_ASSERT(reader.serialize<Trait>(values[index + prev_index], std::forward<Args>(args)...));

				prev_index += index;
			}

			return true;
		}
	};
}