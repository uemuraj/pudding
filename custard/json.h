#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>


struct JsonContext;

class Json
{
	const size_t m_nested;
	std::shared_ptr<JsonContext> m_context;

public:
	Json(std::shared_ptr<JsonContext> context);
	Json(std::wstring_view text);
	~Json() noexcept;

	enum State { Object, Array, Next, End };
	using Value = std::variant<State, std::pair<std::wstring, Json>, Json, std::wstring>;

	Value Parse();

	bool GetBool()
	{
		return std::get<std::wstring>(Parse()) == L"true";
	}

	std::wstring GetString()
	{
		return std::get<std::wstring>(Parse());
	}
};


template <typename T>
class JsonVisitor
{
	T & m_visitor;

public:
	JsonVisitor(T & visitor) : m_visitor(visitor)
	{}

	bool operator()(std::pair<std::wstring, Json> && keyValue)
	{
		if constexpr (std::is_invocable_v<T, std::wstring &&, Json &&>)
		{
			m_visitor(std::move(std::get<0>(keyValue)), std::move(std::get<1>(keyValue)));
		}

		return true;
	}

	bool operator()(Json && value)
	{
		if constexpr (std::is_invocable_v<T, Json &&>)
		{
			m_visitor(std::move(value));
		}

		return true;
	}

	bool operator()(std::wstring && value)
	{
		if constexpr (std::is_invocable_v<T, std::wstring &&>)
		{
			m_visitor(std::move(value));
		}

		return true;
	}

	bool operator()(Json::State state)
	{
		return (state != Json::State::End);
	}
};

template <typename T>
inline void VisitJson(T & visitor, Json & json)
{
	while (std::visit(JsonVisitor<T>(visitor), json.Parse())) /**/;
}

template <typename T>
inline T VisitJson(Json & json)
{
	T visitor;
	while (std::visit(JsonVisitor<T>(visitor), json.Parse())) /**/;
	return visitor;
}
