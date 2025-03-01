#include "json.h"

#include <stdexcept>
#include <stack>

static wchar_t ValidateEscapeChar(wchar_t ch)
{
	switch (ch)
	{
	case L'"':
	case L'\\':
	case L'/':
		return ch;
	case L'b':
		return L'\b';
	case L'f':
		return L'\f';
	case L'n':
		return L'\n';
	case L'r':
		return L'\r';
	case L't':
		return L'\t';
	case L'u':
		return L'\0';
	default:
		throw new std::invalid_argument("Invalid escape character.");
	}
}

static std::uint16_t FromHexChar(wchar_t ch)
{
	static constexpr wchar_t hexChars[] = L"0123456789ABCDEF";

	if (auto pos = wcschr(hexChars, towupper(ch)))
	{
		return (std::uint16_t) (pos - hexChars);
	}

	throw new std::invalid_argument("Invalid hex character.");
}

static std::wstring UnescapeString(const wchar_t * ptr, const wchar_t * end)
{
	std::wstring buff;

	buff.reserve(end - ptr);

	while (ptr < end)
	{
		auto ch = *ptr++;

		if (ch == L'\\')
		{
			ch = ValidateEscapeChar(*ptr++);

			if (ch == L'\0')
			{
				if ((end - ptr) < 4)
				{
					throw new std::invalid_argument("Invalid escape sequence.");
				}

				ch |= FromHexChar(*ptr++);
				ch <<= 4;
				ch |= FromHexChar(*ptr++);
				ch <<= 4;
				ch |= FromHexChar(*ptr++);
				ch <<= 4;
				ch |= FromHexChar(*ptr++);
			}
		}

		buff.push_back(ch);
	}

	return buff;
}

class JsonContext : public std::enable_shared_from_this<JsonContext>
{
	const wchar_t * txt;
	const wchar_t * end;

	enum state { object, array };
	std::stack<state> current;

public:
	JsonContext(std::wstring_view json) : txt(json.data()), end(json.data() + json.size())
	{}

	bool IsObject()
	{
		return SkipWhiteSpace() && (*txt == L'{');
	}

	bool IsArray()
	{
		return SkipWhiteSpace() && (*txt == L'[');
	}

	bool Empty()
	{
		return !SkipWhiteSpace();
	}

	std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Parse()
	{
		if (SkipWhiteSpace())
		{
			switch (*txt++)
			{
			case L'{':
				if (SkipWhiteSpace() && *txt != L'}')
				{
					current.push(state::object);
					return ParseKeyValue();
				}
				break;

			case L'[':
				if (SkipWhiteSpace() && *txt != L']')
				{
					current.push(state::array);
					return Json(shared_from_this());
				}
				break;

			case L'}':
			case L']':
				current.pop();
				break;

			case L',':
				if (SkipWhiteSpace())
				{
					if (*txt == L'}' || *txt == L']')
					{
						current.pop();
						++txt;
					}
					else if (!current.empty() && current.top() == state::object)
					{
						return ParseKeyValue();
					}
					else
					{
						return Json(shared_from_this());
					}
				}
				break;

			case L'"':
				return QuotedString();

			default:
				--txt;
				return UnquotedString();
			}
		}

		return std::monostate();
	}

private:
	bool SkipWhiteSpace()
	{
		while (txt < end)
		{
			if (iswgraph(*txt))
			{
				return true;
			}

			++txt;
		}

		return false;
	}

	bool SkipWhiteSpaceTo(wchar_t ch)
	{
		while (txt < end)
		{
			if (*txt == ch)
			{
				return true;
			}

			if (iswgraph(*txt))
			{
				return false;
			}

			++txt;
		}

		return false;
	}

	std::pair<std::wstring, Json> ParseKeyValue()
	{
		auto key = ParseString();

		if (SkipWhiteSpaceTo(L':'))
		{
			++txt;
			return { key, Json(shared_from_this()) };
		}

		throw new std::invalid_argument("Key value pair expected.");
	}

	std::wstring ParseString()
	{
		if (*txt == L'"')
		{
			++txt;
			return QuotedString();
		}

		return UnquotedString();
	}

	std::wstring QuotedString()
	{
		bool escape = false;
		auto begin = txt;

		while (txt < end)
		{
			if (*txt == L'"')
			{
				if (!escape)
				{
					return { begin, txt++ };
				}

				return UnescapeString(begin, txt++);
			}

			if (*txt++ == L'\\')
			{
				ValidateEscapeChar(*txt++);
				escape = true;
			}
		}

		throw new std::invalid_argument("Unterminated string.");
	}

	std::wstring UnquotedString()
	{
		bool escape = false;
		auto begin = txt;

		while (txt < end)
		{
			if (wcschr(LR"(,[]{})", *txt) || !iswgraph(*txt))
			{
				if (!escape)
				{
					return { begin, txt };
				}

				return UnescapeString(begin, txt);
			}

			if (*txt++ == L'\\')
			{
				ValidateEscapeChar(*txt++);
				escape = true;
			}
		}

		return { begin, end };
	}
};


Json::Json(std::shared_ptr<JsonContext> context) : m_context(context)
{}

Json::Json(std::wstring_view json) : m_context(std::make_shared<JsonContext>(json))
{}

Json::~Json() noexcept
{}

bool Json::IsObject() const
{
	return m_context->IsObject();
}

bool Json::IsArray() const
{
	return m_context->IsArray();
}

bool Json::Empty() const
{
	return m_context->Empty();
}

std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Json::Parse()
{
	return m_context->Parse();
}
