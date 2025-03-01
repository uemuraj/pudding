#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
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
	bool Empty() const;

	std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Parse();
};

// TODO: u8string ‚ğ’¼Úˆ—‚Å‚«‚é‚æ‚¤‚É‚·‚é
