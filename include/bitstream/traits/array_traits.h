#pragma once
#include "../utility/assert.h"

#include "../stream/serialize_traits.h"
#include "../stream/bit_reader.h"
#include "../stream/bit_writer.h"

#include "../traits/bool_trait.h"
#include "../traits/integral_traits.h"

#include <cstdint>

namespace bitstream
{
	template<typename T, typename = T>
	struct array_subset;

	/**
	 * @brief A trait used for serializing a subset of an array of objects
	 * @tparam T The type of the object in the array
	 * @tparam Trait 
	*/
	template<typename T, typename Trait>
	struct serialize_traits<array_subset<T, Trait>>
	{
    private:
        template<uint32_t Min, uint32_t Max, typename Stream>
        static bool serialize_difference(Stream& stream, int& previous, int& current, uint32_t& difference)
        {
            bool use_bits;
            if constexpr (Stream::writing)
                use_bits = difference <= Max;
            BS_ASSERT(stream.template serialize<bool>(use_bits));
            if (use_bits)
            {
                using bounded_trait = bounded_int<uint32_t, Min, Max>;

                BS_ASSERT(stream.template serialize<bounded_trait>(difference));
                if constexpr (Stream::reading)
                    current = previous + difference;
                previous = current;
                return true;
            }

            return false;
        }

        template<typename Stream>
        static bool serialize_index(Stream& stream, int& previous, int& current, int max_size)
        {
            uint32_t difference;
            if constexpr (Stream::writing)
            {
                BS_ASSERT(previous < current);
                difference = current - previous;
                BS_ASSERT(difference > 0);
            }

            // +1 (1 bit)
            bool plus_one;
            if constexpr (Stream::writing)
                plus_one = difference == 1;
            BS_ASSERT(stream.template serialize<bool>(plus_one));
            if (plus_one)
            {
                if constexpr (Stream::reading)
                    current = previous + 1;
                previous = current;
                return true;
            }

            // [+2,5] -> [0,3] (2 bits)
            if (serialize_difference<2, 5>(stream, previous, current, difference))
                return true;

            // [6,13] -> [0,7] (3 bits)
            if (serialize_difference<6, 13>(stream, previous, current, difference))
                return true;

            // [14,29] -> [0,15] (4 bits)
            if (serialize_difference<14, 29>(stream, previous, current, difference))
                return true;

            // [30,61] -> [0,31] (5 bits)
            if (serialize_difference<30, 61>(stream, previous, current, difference))
                return true;

            // [62,125] -> [0,63] (6 bits)
            if (serialize_difference<62, 125>(stream, previous, current, difference))
                return true;

            // [126,MaxObjects+1] 
            BS_ASSERT(stream.template serialize<uint32_t>(difference, 126, max_size));
            if constexpr (Stream::reading)
                current = previous + difference;
            previous = current;
            return true;
        }

    public:
		/**
		 * @brief Writes a subset of the array @p values into the writer
		 * @tparam Compare A function type which returns a bool
		 * @tparam ...Args The types of any additional arguments
		 * @param writer The stream to write to
		 * @param values The array of objects to serialize
		 * @param max_size The size of the array
		 * @param compare A function which returns true if the object should be written, false otherwise
		 * @param ...args Any additional arguments to use when serializing each individual object
		 * @return Success
		*/
		template<typename Compare, typename... Args>
		static bool serialize(bit_writer& writer, T* values, int max_size, Compare compare, Args&&... args) noexcept
		{
			int prev_index = -1;
			for (int index = 0; index < max_size; index++)
			{
				if (!compare(values[index]))
					continue;

                BS_ASSERT(serialize_index(writer, prev_index, index, max_size));

				BS_ASSERT(writer.serialize<Trait>(values[index], std::forward<Args>(args)...));
			}

            BS_ASSERT(serialize_index(writer, prev_index, max_size, max_size));

			return true;
		}

		/**
		 * @brief Writes a subset of a serialized array into @p values
		 * @tparam Compare A stub. Not used on read
		 * @tparam ...Args The types of any additional arguments
		 * @param reader The stream to read from
		 * @param values The array of objects to read into
		 * @param max_size The size of the array
		 * @param compare Not used, but kept for compatibility with the serialize write function
		 * @param ...args Any additional arguments to use when serializing each individual object
		 * @return Success
		*/
		template<typename Compare, typename... Args>
		static bool serialize(bit_reader& reader, T* values, int max_size, Compare compare, Args&&... args) noexcept
		{
			int prev_index = -1;
            int index = 0;
			while (true)
			{
                BS_ASSERT(serialize_index(reader, prev_index, index, max_size));

				if (index == max_size)
					break;

				BS_ASSERT(reader.serialize<Trait>(values[index], std::forward<Args>(args)...));
			}

			return true;
		}
	};
}