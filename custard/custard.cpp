#include "custard.h"
#include <Windows.h>
#include <winhttp.h>

#include <format>
#include <functional>
#include <unordered_map>
#include <stack>

namespace
{
	struct Handle
	{
		HINTERNET m_handle;

		Handle(HINTERNET handle) noexcept : m_handle(handle)
		{}

		~Handle() noexcept
		{
			if (m_handle)
			{
				::WinHttpCloseHandle(m_handle);
			}
		}

		operator HINTERNET() const noexcept
		{
			return m_handle;
		}
	};

	struct Session : Handle
	{
		Session() : Handle(::WinHttpOpen(L"A WinHTTP Program Custard/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0))
		{
			if (!m_handle)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpen");
			}
		}
	};

	struct Connection : Handle
	{
		Connection(HINTERNET session, const wchar_t * server) : Handle(::WinHttpConnect(session, server, INTERNET_DEFAULT_HTTPS_PORT, 0))
		{
			if (m_handle == nullptr)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpConnect");
			}
		}
	};

	class Request : Handle
	{
	public:
		Request(HINTERNET connection, const wchar_t * verb, const wchar_t * path) : Handle(::WinHttpOpenRequest(connection, verb, path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE))
		{
			if (m_handle == nullptr)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpenRequest");
			}
		}

		void Send(const wchar_t * headers, void * data, uint32_t size)
		{
			if (!::WinHttpSendRequest(m_handle, headers, -1, data, size, size, 0))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpSendRequest");
			}
		}

		void Recv()
		{
			if (!::WinHttpReceiveResponse(m_handle, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReceiveResponse");
			}
		}

		int StatusCode()
		{
			DWORD code = 0;
			DWORD size = sizeof(code);

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &code, &size, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryHeaders");
			}

			return (int) code;
		}

		std::unordered_map<std::wstring, std::wstring> ResponseHeaders()
		{
			DWORD size = 0;
			DWORD count = 0;

			if (auto error = ::WinHttpQueryHeadersEx(m_handle, WINHTTP_QUERY_RAW_HEADERS, 0ull, 0, nullptr, nullptr, nullptr, &size, nullptr, &count); error != ERROR_INSUFFICIENT_BUFFER)
			{
				throw std::system_error(error, std::system_category(), "WinHttpQueryHeadersEx");
			}

			auto buff = std::make_unique<BYTE[]>(size);

			WINHTTP_EXTENDED_HEADER * headers = nullptr;

			if (auto error = ::WinHttpQueryHeadersEx(m_handle, WINHTTP_QUERY_RAW_HEADERS, 0ull, 0, nullptr, nullptr, buff.get(), &size, &headers, &count); error != ERROR_SUCCESS)
			{
				throw std::system_error(error, std::system_category(), "WinHttpQueryHeadersEx");
			}

			std::unordered_map<std::wstring, std::wstring> map;

			for (auto & header : std::ranges::subrange(headers, headers + count))
			{
				map[header.pwszName] = header.pwszValue;
			}

			return map;
		}

		std::vector<std::byte> ResponseData()
		{
			std::vector<std::byte> buff;

			size_t offset = 0;

			for (auto size = GetAvailableDataSize(), read = 0ul; size > 0; size = GetAvailableDataSize(), offset += read)
			{
				buff.resize(offset + size);

				if (!::WinHttpReadData(m_handle, buff.data() + offset, size, &read))
				{
					throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReadData");
				}

				if (read != size)
				{
					throw std::system_error(ERROR_HANDLE_EOF, std::system_category(), "WinHttpReadData");
				}
			}

			return buff;
		}

	private:
		DWORD GetAvailableDataSize()
		{
			DWORD size = 0;

			if (!::WinHttpQueryDataAvailable(m_handle, &size))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryDataAvailable");
			}

			return size;
		}
	};
}

//
// https://api.slack.com/tutorials/tracks/posting-messages-with-curl
//

class CustardContext
{
	Session m_session;
	Connection m_connection;
	std::wstring m_headers;

public:
	CustardContext(const wchar_t * server, std::wstring_view token) : m_connection(m_session, server)
	{
		m_headers = std::format(L"Authorization: Bearer {}\r\n", token);
		m_headers += L"Content-Type: application/x-www-form-urlencoded\r\n";
	}

	void Post(const wchar_t * path, std::wstring_view data)
	{
		auto content = ConvertFrom(data);

		Request request(m_connection, L"POST", path);
		request.Send(m_headers.c_str(), content.data(), (uint32_t) content.size());
		request.Recv();

		// TODO: ステータスコードが 200 以外の場合、レスポンスデータがあれば見て、適当な内容の例外を投げる
		// TODO: レスポンスヘッダが "Content-Type: application/json" であることを確認してからレスポンスデータを解析する
		// TODO: レスポンスが {"ok":false} の場合、適当な内容の例外を投げる
#if defined(_DEBUG)
		::OutputDebugStringW(L"=== Response Headers ===\r\n");

		for (auto & [name, value] : request.ResponseHeaders())
		{
			::OutputDebugStringW(std::format(L"{}: {}\n", name, value).c_str());
		}

		::OutputDebugStringW(L"=== Response Data ===\r\n");
		::OutputDebugStringW(ConvertFrom(request.ResponseData()).c_str());
		::OutputDebugStringW(L"\r\n=====================\r\n");
#endif
	}
};


Custard::Custard(std::wstring_view token) : m_context(std::make_unique<CustardContext>(L"slack.com", token))
{}

Custard::~Custard() noexcept
{}

void Custard::PostToSlack(std::wstring_view channel, std::wstring_view message)
{
	m_context->Post(L"/api/chat.postMessage", std::format(L"channel={}&text={}", channel, message));
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
			return { begin, txt++ };
		}

		throw new std::invalid_argument("Unterminated string.");
	}

	std::wstring ParseUnquotedString()
	{
		auto begin = txt;

		if (SkipToEndUnquotedString())
		{
			return { begin, txt };
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

				if (wcschr(LR"("\/bfnrtu)", *txt) == nullptr)
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

				if (wcschr(LR"("\/bfnrtu)", *txt) == nullptr)
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

std::variant<std::monostate, std::pair<std::wstring, Json>, Json, std::wstring> Json::Parse()
{
	return m_context->Parse();
}


std::u8string ConvertFrom(std::wstring_view wstr)
{
	const auto wstr_size = (int) wstr.size();
	const auto utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr_size, nullptr, 0, nullptr, nullptr);

	if (utf8_size <= 0)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte");
	}

	std::u8string utf8(utf8_size, '\0');

	if (::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr_size, (char *) utf8.data(), utf8_size, nullptr, nullptr) == 0)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte");
	}

	return utf8;
}

std::wstring ConvertFrom(std::u8string_view utf8)
{
	const auto utf8_size = (int) utf8.size();
	const auto wstr_size = ::MultiByteToWideChar(CP_UTF8, 0, (const char *) utf8.data(), utf8_size, nullptr, 0);

	if (wstr_size <= 0)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "MultiByteToWideChar");
	}

	std::wstring wstr(wstr_size, L'\0');

	if (::MultiByteToWideChar(CP_UTF8, 0, (const char *) utf8.data(), utf8_size, wstr.data(), wstr_size) == 0)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "MultiByteToWideChar");
	}

	return wstr;
}
