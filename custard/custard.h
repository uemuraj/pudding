#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <utility>
#include <variant>


class CustardContext;

class Custard
{
	std::unique_ptr<CustardContext> m_context;

public:
	Custard(std::wstring_view token);
	~Custard() noexcept;

	void PostToSlack(std::wstring_view channel, std::wstring_view message);
};


class JsonContext;

class Json
{
	std::shared_ptr<JsonContext> m_context;

public:
	Json(std::shared_ptr<JsonContext> context);
	Json(std::wstring_view json);
	~Json() noexcept;

	std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Parse();
};


std::u8string ConvertFrom(std::wstring_view wstr);

std::wstring ConvertFrom(std::u8string_view utf8);

inline std::wstring ConvertFrom(const std::vector<std::byte> & data)
{
	return ConvertFrom({ (const char8_t *) data.data(), data.size() });
}
