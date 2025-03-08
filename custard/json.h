#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

// TODO: u8string ‚ğ’¼Úˆ—‚Å‚«‚é‚æ‚¤‚É‚·‚é

class JsonContext;

class Json
{
	std::shared_ptr<JsonContext> m_context;

	int m_nesting;

public:
	Json(std::shared_ptr<JsonContext> context);
	Json(std::wstring_view json);
	~Json() noexcept;

	enum State { Object, Array, Next, End };
	using Value = std::variant<State, std::pair<std::wstring, Json>, Json, std::wstring>;

	Value Parse();
};
