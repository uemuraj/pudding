#pragma once

#include <string>
#include <string_view>
#include <vector>

std::u8string ConvertFrom(std::wstring_view wstr);

std::wstring ConvertFrom(std::u8string_view utf8);

inline std::wstring ConvertFrom(const std::vector<std::byte> & data)
{
	return ConvertFrom({ (const char8_t *) data.data(), data.size() });
}
