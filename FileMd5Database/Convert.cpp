
#include <charconv>
#include <sstream>

#include "Convert.h"

template <typename ...Args>
std::string __Convert_Combine__(Args&&... args)
{
	std::ostringstream ss{};
	(ss << ... << args);
	return ss.str();
}

template<typename T>
T __String_Stream_Convert__(const std::string& string)
{
	T res;
	std::istringstream(string) >> res;
	return res;
}

#define __Convert_ToStringFunc__(x) #x
#define __Convert_ToString__(x) __Convert_ToStringFunc__(x)
#define __Convert_Line__ __Convert_ToString__(__LINE__)

#define __Convert_ThrowEx__(...) throw std::runtime_error(__Convert_Combine__( __FILE__ ": " __Convert_Line__ ": ", __func__, ": ", __VA_ARGS__))

namespace Convert
{
	int ToInt(const std::string& value, const int base)
	{
		int res;
		auto [p, e] = std::from_chars(value.data(), value.data() + value.length(), res, base);
		if (e != std::errc{}) __Convert_ThrowEx__("convert error: invalid literal: ", p);
		return res;
	}

	uint64_t ToUint64(const std::string& value, const int base)
	{
		char* end;
		return std::strtoull(value.c_str(), &end, base);
	}
	
	float ToFloat(const std::string& value)
	{
		return __String_Stream_Convert__<float>(value);
	}

	double ToDouble(const std::string& value)
	{
		return __String_Stream_Convert__<double>(value);
	}

	std::string ToString(const std::string_view& string)
	{
		return std::string(string);
	}
}

#undef __Convert_ToStringFunc__
#undef __Convert_ToString__
#undef __Convert_Line__
#undef __Convert_ThrowEx__