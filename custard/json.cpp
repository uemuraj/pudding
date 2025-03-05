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

	std::stack<Json::State> current;
	bool firstElement;

public:
	JsonContext(std::wstring_view json) : txt(json.data()), end(json.data() + json.size()), firstElement(false)
	{}

	std::variant<Json::State, std::pair<std::wstring, Json>, Json, std::wstring> Parse(int & nesting)
	{
		if (SkipWhiteSpace())
		{
			switch (*txt++)
			{
			case L'{':
				current.push(Json::State::Object), ++nesting;
				firstElement = true;
				return Json::State::Object;

			case L'[':
				current.push(Json::State::Array), ++nesting;
				firstElement = true;
				return Json::State::Array;

			case L'}':
			case L']':
				current.pop(), --nesting;
				return (nesting > 0) ? Json::State::Next : Json::State::End;

			case L',':
				return Next(nesting);

			case L'"':
				if (std::exchange(firstElement, false))
				{
					--txt;
					return Next(nesting);
				}
				return QuotedString();

			default:
				--txt;
				if (std::exchange(firstElement, false))
				{
					return Next(nesting);
				}
				return UnquotedString();
			}
		}

		return Json::State::End;
	}

private:
	std::variant<Json::State, std::pair<std::wstring, Json>, Json, std::wstring> Next(int & nesting)
	{
		if (SkipWhiteSpace())
		{
			if (*txt == L'}' || *txt == L']')
			{
				++txt;
				current.pop(), --nesting;
				return (nesting > 0) ? Json::State::Next : Json::State::End;
			}

			if (current.empty() || current.top() == Json::State::Array)
			{
				return Parse(nesting);
			}
			else
			{
				return ParseKeyValue();
			}
		}

		return Json::State::End;
	}

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


Json::Json(std::shared_ptr<JsonContext> context) : m_context(context), m_nesting(0)
{}

Json::Json(std::wstring_view json) : m_context(std::make_shared<JsonContext>(json)), m_nesting(0)
{}

Json::~Json() noexcept
{}

std::variant<Json::State, std::pair<std::wstring, Json>, Json, std::wstring> Json::Parse()
{
	return m_context->Parse(m_nesting);
}
