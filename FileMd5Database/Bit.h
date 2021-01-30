#pragma once

#include <cstdint>
#include <type_traits>
#include <climits>

template<typename T, std::size_t... N>
constexpr T __EndianSwapImpl__(T i, std::index_sequence<N...>)
{
	return (((i >> N * CHAR_BIT & static_cast<std::uint8_t>(-1)) << (sizeof(T) - 1 - N) * CHAR_BIT) | ...);
}

namespace Bit
{
	enum class Endian
	{
#ifdef _WIN32
		Little = 0,
		Big = 1,
		Native = Little
#else
		Little = __ORDER_LITTLE_ENDIAN__,
		Big = __ORDER_BIG_ENDIAN__,
		Native = __BYTE_ORDER__
#endif
	};
	
	template<typename T, typename U = std::make_unsigned_t<T>>
	constexpr U EndianSwap(T i)
	{
		return __EndianSwapImpl__<U>(i, std::make_index_sequence<sizeof(T)>{});
	}
}
