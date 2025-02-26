#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <variant>

class JsonContext;

class Json
{
	std::shared_ptr<JsonContext> m_context;

public:
	Json(std::shared_ptr<JsonContext> context);
	Json(std::wstring_view json);
	~Json() noexcept;

	bool IsObject() const;
	bool IsArray() const;

	std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Parse();
};
