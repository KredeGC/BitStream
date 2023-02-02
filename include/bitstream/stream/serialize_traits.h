#pragma once

namespace bitstream
{
	/**
	 * @brief A class for specializing trait serialization functions
	 * @tparam Trait Make a specialization on this type
	 * @tparam Void Use std::enable_if here if you need to, otherwise leave empty
	*/
	template<typename Trait, typename Void = void>
	struct serialize_traits;
}