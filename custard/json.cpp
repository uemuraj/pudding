#include "json.h"

#include <stdexcept>
#include <cwchar>
#include <cctype>
#include <stack>

static wchar_t DecodeUnicodeEscapeSequence(const wchar_t *& ptr, const wchar_t * end)
{
	if ((end - ptr) >= 4)
	{
		static constexpr wchar_t hex[] = L"0123456789ABCDEF";

		wchar_t ch = 0;

		for (int i = 0; i < 4; ++i)
		{
			if (auto pos = wcschr(hex, towupper(*ptr++)))
			{
				ch <<= 4;
				ch |= (unsigned char) (pos - hex);
			}
			else
			{
				throw new std::invalid_argument("Invalid escape sequence.");
			}
		}

		return ch;
	}

	throw new std::invalid_argument("Invalid escape sequence.");
}

static std::wstring UnescapeString(const wchar_t * ptr, const wchar_t * end)
{
	std::wstring buff;

	buff.reserve(end - ptr);

	while (ptr < end)
	{
		if (*ptr == L'\\')
		{
			++ptr;

			switch (*ptr++)
			{
			case L'"':
				buff.push_back(L'"');
				continue;
			case L'/':
				buff.push_back(L'/');
				continue;
			case L'\\':
				buff.push_back(L'\\');
				continue;
			case L'b':
				buff.push_back(L'\b');
				continue;
			case L'f':
				buff.push_back(L'\f');
				continue;
			case L'n':
				buff.push_back(L'\n');
				continue;
			case L'r':
				buff.push_back(L'\r');
				continue;
			case L't':
				buff.push_back(L'\t');
				continue;
			case L'u':
				buff.push_back(DecodeUnicodeEscapeSequence(ptr, end));
				continue;
			default:
				throw new std::invalid_argument("Invalid escape sequence.");
			}
		}

		buff.push_back(*ptr++);
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
				return ParseQuotedString();

			default:
				--txt;
				return ParseUnquotedString();
			}
		}

		return std::monostate();
	}

private:
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

			return ParseQuotedString();
		}
		else
		{
			return ParseUnquotedString();
		}
	}

	std::wstring ParseQuotedString()
	{
		auto begin = txt;

		if (SkipToEndOfQuotedString())
		{
			return UnescapeString(begin, txt++);
		}

		throw new std::invalid_argument("Unterminated string.");
	}

	std::wstring ParseUnquotedString()
	{
		auto begin = txt;

		if (SkipToEndUnquotedString())
		{
			return UnescapeString(begin, txt);
		}

		return { begin, end };
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

	bool SkipToEndOfQuotedString()
	{
		while (txt < end)
		{
			if (*txt == L'"')
			{
				return true;
			}

			if (*txt == L'\\')
			{
				++txt;

				if (wcschr(LR"("/\bfnrtu)", *txt) == nullptr)
				{
					throw new std::invalid_argument("Invalid escape sequence.");
				}
			}

			++txt;
		}

		return false;
	}

	bool SkipToEndUnquotedString()
	{
		while (txt < end)
		{
			if (wcschr(LR"(,[]{})", *txt))
			{
				return true;
			}

			if (!iswgraph(*txt))
			{
				return true;
			}

			if (*txt == L'\\')
			{
				++txt;

				if (wcschr(LR"("/\bfnrtu)", *txt) == nullptr)
				{
					throw new std::invalid_argument("Invalid escape sequence.");
				}
			}

			++txt;
		}

		return false;
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

std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Json::Parse()
{
	return m_context->Parse();
}
