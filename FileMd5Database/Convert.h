#pragma once

#include <string>
#include <charconv>

#define __Convert_ToStringFunc__(x) #x
#define __Convert_ToString__(x) __Convert_ToStringFunc__(x)
#define __Convert_Line__ __Convert_ToString__(__LINE__)

template<typename...Args>
std::string __Convert_Combine__(Args&&... args)
{
	std::string res;
	(res.append(args), ...);
	return res;
}

#define __Convert_ThrowEx__(...) throw std::runtime_error(__Convert_Combine__(__FILE__ ": " __Convert_Line__ ": ", __func__, ": ", __VA_ARGS__))

template<typename T, typename Args>
[[nodiscard]] T __From_String_Impl__(const std::string& value, const Args args)
{
	T res;
	auto [p, e] = std::from_chars(value.c_str(), value.c_str() + value.length(), res, args);
	if (e != std::errc{}) __Convert_ThrowEx__("convert error: invalid literal: ", p);
	return res;
}

template<typename T, typename...Args>
[[nodiscard]] std::string __To_String_Impl__(const T& value, Args&& ... args)
{
	char res[65] = { 0 };
	auto [p, e] = std::to_chars(res, res + 65, value, std::forward<Args>(args)...);
	if (e != std::errc{}) __Convert_ThrowEx__("convert error: invalid literal: ", p);
	return res;
}

namespace Convert
{
	template<typename T,std::enable_if_t<std::negation_v<typename std::disjunction<
			typename std::is_integral<T>::value,
			typename std::is_floating_point<T>::value
		>::value>, int> = 0>
	[[nodiscard]] std::string ToString(const T& value)
	{
		return std::string(value);
	}
	
	template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	[[nodiscard]] std::string ToString(const T value, const int base = 10)
	{
		return __To_String_Impl__<T>(value, base);
	}
	
	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	[[nodiscard]] std::string ToString(const T value)
	{
		return __To_String_Impl__<T>(value);
	}

#ifdef _MSC_VER
	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	[[nodiscard]] std::string ToString(const T value, const std::chars_format& fmt)
	{
		return __To_String_Impl__<T>(value, fmt);
	}
	
	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	[[nodiscard]] std::string ToString(const T value, const std::chars_format& fmt, const int precision)
	{
		return __To_String_Impl__<T>(value, fmt, precision);
	}
#endif
	
	template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	[[nodiscard]] T FromString(const std::string& value, const int base = 10)
	{
		return __From_String_Impl__<T>(value, base);
	}

#ifdef _MSC_VER
	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	[[nodiscard]] T FromString(const std::string& value, const std::chars_format& fmt = std::chars_format::general)
	{
		return __From_String_Impl__<T>(value, fmt);
	}
#endif
}

#undef __Convert_ToStringFunc__
#undef __Convert_ToString__
#undef __Convert_Line__
#undef __Convert_ThrowEx__
