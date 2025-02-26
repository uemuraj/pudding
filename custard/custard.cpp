#include <Windows.h>
#include <winhttp.h>

#include <format>
#include <unordered_map>
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
