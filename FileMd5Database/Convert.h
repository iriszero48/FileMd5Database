#pragma once

#include <string>
#include <charconv>

namespace Convert
{
	[[nodiscard]] int ToInt(const std::string& value, int base = 10);

	[[nodiscard]] uint64_t ToUint64(const std::string& value, int base = 10);

	[[nodiscard]] float ToFloat(const std::string& value);

	[[nodiscard]] double ToDouble(const std::string& value);

	template<typename T = uint8_t>
	[[nodiscard]] std::string ToString(const T value, const int base = 10)
	{
		char res[65] = { 0 };
		auto [p, e] = std::to_chars(res, res + 65, value, base);
		if (e != std::errc{}) throw std::runtime_error("convert error");
		return res;
	}
	
	[[nodiscard]] std::string ToString(const std::string_view& string);
}
