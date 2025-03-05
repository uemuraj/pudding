#include <Windows.h>
#include <winhttp.h>

#include <format>
#include <optional>
#include <system_error>

#include "custard.h"
#include "json.h"
#include "wconv.h"

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

		std::wstring ResponseHeaders()
		{
			DWORD size = 0;

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_RAW_HEADERS_CRLF, nullptr, nullptr, &size, nullptr))
			{
				if (auto error = GetLastError(); error != ERROR_INSUFFICIENT_BUFFER)
				{
					throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
				}
			}

			std::wstring buffer(size / sizeof(wchar_t) - 1, L'\0');

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_RAW_HEADERS_CRLF, nullptr, buffer.data(), &size, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryHeaders");
			}

			return buffer;
		}

		std::wstring ContentType()
		{
			DWORD size = 0;

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_TYPE, nullptr, nullptr, &size, nullptr))
			{
				if (auto error = GetLastError(); error != ERROR_INSUFFICIENT_BUFFER)
				{
					throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
				}
			}

			std::wstring buff(size / sizeof(wchar_t) - 1, L'\0');

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_TYPE, nullptr, buff.data(), &size, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryHeaders");
			}

			return buff;
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

struct SlackResult
{
	std::optional<bool> ok;

	void operator()(Json::State)
	{
		// do nothing
	}

	void operator()(Json & json)
	{
		auto parsed = json.Parse();
		std::visit(*this, parsed);
	}

	void operator()(std::pair<std::wstring, Json> & pair)
	{
		auto & [key, value] = pair;

		if (key == L"ok")
		{
			ok = (std::get<std::wstring>(value.Parse()) == L"true");
		}
	}

	void operator()(std::wstring & str)
	{
#if defined(_DEBUG)
		::OutputDebugStringW(str.c_str());
		::OutputDebugStringW(L"\r\n");
#endif
	}
};

class CustardContext
{
	Session m_session;
	Connection m_connection;
	std::wstring m_request;

public:
	CustardContext(const wchar_t * server, std::wstring_view token) : m_connection(m_session, server)
	{
		m_request = std::format(L"Authorization: Bearer {}\r\n", token);
		m_request += L"Content-Type: application/x-www-form-urlencoded\r\n";
	}

	bool Post(const wchar_t * path, std::wstring_view data)
	{
		auto content = ConvertFrom(data);

		Request request(m_connection, L"POST", path);
		request.Send(m_request.c_str(), content.data(), (uint32_t) content.size());
		request.Recv();

#if defined(_DEBUG)
		::OutputDebugStringW(L"=== Response ===\r\n");
		::OutputDebugStringW(request.ResponseHeaders().c_str());
#endif
		auto response = request.ResponseData();

		if (request.ContentType().starts_with(L"application/json"))
		{
			auto text = ConvertFrom(response);
			auto json = Json(text);

			SlackResult result;
			result(json);

			if (result.ok.has_value())
			{
				return result.ok.value();
			}
		}

		// TODO: ó·äOÇ…ìKêÿÇ»èÓïÒÇä‹ÇﬂÇÈ

		throw std::runtime_error("Unexpected response.");
	}
};


Custard::Custard(std::wstring_view token) : m_context(std::make_unique<CustardContext>(L"slack.com", token))
{}

Custard::~Custard() noexcept
{}

bool Custard::PostToSlack(std::wstring_view channel, std::wstring_view message)
{
	return m_context->Post(L"/api/chat.postMessage", std::format(L"channel={}&text={}", channel, message));
}
