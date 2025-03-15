#include <Windows.h>
#include <winhttp.h>

#include <format>
#include <optional>
#include <functional>
#include <system_error>

#include "custard.h"
#include "wconv.h"
#include "json.h"

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

		Handle(const Handle &) = delete;
		Handle(Handle && other) = delete;

		Handle & operator=(const Handle &) = delete;
		Handle & operator=(Handle && other) = delete;
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
		Connection(HINTERNET session, const wchar_t * host) : Handle(::WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0))
		{
			if (m_handle == nullptr)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpConnect");
			}
		}
	};

	struct Request : Handle
	{
		Request(HINTERNET connection, const wchar_t * verb, const wchar_t * path) : Handle(::WinHttpOpenRequest(connection, verb, path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE))
		{
			if (m_handle == nullptr)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpenRequest");
			}
		}

		void Send(const wchar_t * headers, void * content, uint32_t size)
		{
			if (!::WinHttpSendRequest(m_handle, headers, -1, content, size, size, 0))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpSendRequest");
			}
		}
	};

	struct Response : Handle
	{
		Response(Request & request) : Handle(std::exchange(request.m_handle, nullptr))
		{
			if (!::WinHttpReceiveResponse(m_handle, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReceiveResponse");
			}
		}

		std::wstring Headers()
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

		std::wstring Type()
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

		std::wstring Encoding()
		{
			DWORD size = 0;

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING, nullptr, nullptr, &size, nullptr))
			{
				if (auto error = GetLastError(); error == ERROR_WINHTTP_HEADER_NOT_FOUND)
				{
					return {};
				}
				else if (error != ERROR_INSUFFICIENT_BUFFER)
				{
					throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
				}
			}

			std::wstring buff(size / sizeof(wchar_t) - 1, L'\0');

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING, nullptr, buff.data(), &size, nullptr))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryHeaders");
			}

			return buff;
		}

		void Recv(std::function<void(std::byte *, uint32_t)> callback)
		{
			std::vector<std::byte> buff;

			for (auto size = GetAvailableDataSize(), read = 0ul; size > 0; size = GetAvailableDataSize())
			{
				buff.clear();
				buff.resize(size);

				if (!::WinHttpReadData(m_handle, buff.data(), size, &read))
				{
					throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReadData");
				}

				if (read != size)
				{
					throw std::system_error(ERROR_HANDLE_EOF, std::system_category(), "WinHttpReadData");
				}

				callback(buff.data(), read);
			}
		}

		std::vector<std::byte> Data()
		{
			std::vector<std::byte> buff;

			ReserveConentLength(buff);

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
		void ReserveConentLength(std::vector<std::byte> & buff)
		{
			constexpr size_t minimum = 16 * 1024;
			constexpr size_t maximum = 16 * 1024 * 1024;

			DWORD size = sizeof(DWORD);
			DWORD length = 0;

			if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &length, &size, nullptr))
			{
				if (auto error = ::GetLastError(); error != ERROR_WINHTTP_HEADER_NOT_FOUND)
				{
					throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
				}

				buff.reserve(minimum);
				return;
			}

			if (0 < length && length <= maximum)
			{
				buff.reserve(length);
				return;
			}

			throw std::runtime_error("Content-Length is too large.");
		}

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

	class UrlCracker : URL_COMPONENTS
	{
		std::wstring m_url;

	public:
		UrlCracker(std::wstring_view url) : m_url(url),
			URL_COMPONENTS{ .dwStructSize = sizeof(URL_COMPONENTS), .dwHostNameLength = DWORD(-1), .dwUrlPathLength = DWORD(-1) }
		{
			if (!::WinHttpCrackUrl(m_url.data(), (DWORD) m_url.size(), 0, this))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WinHttpCrackUrl");
			}

			if (nScheme != INTERNET_SCHEME_HTTPS)
			{
				throw std::runtime_error("Unsupported scheme.");
			}

			if (nPort != INTERNET_DEFAULT_HTTPS_PORT)
			{
				throw std::runtime_error("Unsupported port.");
			}

			svHost = { lpszHostName, dwHostNameLength };

			svPath = { lpszUrlPath, dwUrlPathLength };
		}

	protected:
		std::wstring_view svHost;

		std::wstring_view svPath;

	public:
		std::wstring Host() const
		{
			return { lpszHostName, dwHostNameLength };
		}

		std::wstring Path() const
		{
			return { lpszUrlPath, dwUrlPathLength };
		}
	};

	class Https
	{
		Session m_session;
		Connection m_connection;
		std::wstring m_headers;

	public:
		Https(const wchar_t * host) : m_connection(m_session, host)
		{}

		void SetBearerToken(std::wstring_view token)
		{
			m_headers = std::format(L"Authorization: Bearer {}\r\n", token);
		}

		void AddContentType(std::wstring_view type)
		{
			m_headers += std::format(L"Content-Type: {}\r\n", type);
		}

		Response Get(const wchar_t * path)
		{
			Request request(m_connection, L"GET", path);
			request.Send(m_headers.c_str(), nullptr, 0);
			return Response(request);
		}

		Response Post(const wchar_t * path, void * content, uint32_t size)
		{
			Request request(m_connection, L"POST", path);
			request.Send(m_headers.c_str(), content, size);
			return Response(request);
		}
	};

	class FileDownloader : UrlCracker, public Https
	{
		std::wstring_view m_fileName;

	public:
		FileDownloader(std::wstring_view url) : UrlCracker(url), Https(Host().c_str())
		{
			if (auto pos = svPath.find_last_of(L'/'); pos != std::wstring_view::npos)
			{
				m_fileName = svPath.substr(pos + 1);
			}
			else
			{
				throw std::runtime_error("Invalid URL.");
			}
		}

		std::filesystem::path SaveTo(const std::filesystem::path & dest)
		{
			std::error_code ec;

			if (!std::filesystem::is_directory(dest, ec))
			{
				throw std::system_error(ec, "std::filesystem::is_directory");
			}

			auto response = Https::Get(Path().c_str());

#if defined(_DEBUG)
			::OutputDebugStringW(L"=== Response ===\r\n");
			::OutputDebugStringW(response.Headers().c_str());
#endif
			auto file = dest / m_fileName;
			response.Recv(FileWriter(file));
			return file;
		}

	private:
		struct FileWriter
		{
			HANDLE m_handle;

			FileWriter(const std::filesystem::path & path) : m_handle(::CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr))
			{
				if (m_handle == INVALID_HANDLE_VALUE)
				{
					throw std::system_error(::GetLastError(), std::system_category(), "CreateFileW");
				}
			}

			FileWriter(const FileWriter & other) : m_handle(INVALID_HANDLE_VALUE)
			{
				if (!::DuplicateHandle(::GetCurrentProcess(), other.m_handle, ::GetCurrentProcess(), &m_handle, 0, false, DUPLICATE_SAME_ACCESS))
				{
					throw std::system_error(::GetLastError(), std::system_category(), "DuplicateHandle");
				}
			}

			~FileWriter() noexcept
			{
				::CloseHandle(m_handle);
			}

			void operator()(std::byte * data, uint32_t size)
			{
				DWORD written = 0;

				do
				{
					if (!::WriteFile(m_handle, data, size, &written, nullptr))
					{
						throw std::system_error(::GetLastError(), std::system_category(), "WriteFile");
					}

					if (size <= written)
					{
						return;
					}

					data += written;
					size -= written;

				}
				while (written > 0);
			}
		};
	};
}


//
// https://api.slack.com/tutorials/tracks/posting-messages-with-curl
//

struct SlackResponse
{
	std::optional<bool> ok;

	struct Bot
	{
		std::wstring id;
		std::wstring app_id;
		std::wstring name;

		struct Icons
		{
			std::wstring image_36;
			std::wstring image_48;
			std::wstring image_76;

			void operator()(std::wstring && key, Json && value)
			{
				if (key == L"image_36")
				{
					image_36 = value.GetString();
					return;
				}
				if (key == L"image_48")
				{
					image_48 = value.GetString();
					return;
				}
				if (key == L"image_76")
				{
					image_76 = value.GetString();
					return;
				}
			}
		} icons;

		void operator()(std::wstring && key, Json && value)
		{
			if (key == L"id")
			{
				id = value.GetString();
				return;
			}
			if (key == L"app_id")
			{
				app_id = value.GetString();
				return;
			}
			if (key == L"name")
			{
				name = value.GetString();
				return;
			}
			if (key == L"icons")
			{
				VisitJson(icons, value);
				return;
			}
		}
	} bot;

	void operator()(std::wstring && key, Json && value)
	{
		if (key == L"ok")
		{
			ok = value.GetBool();
			return;
		}
		if (key == L"bot" || key == L"bot_profile")
		{
			VisitJson(bot, value);
			return;
		}
	}
};


class SlackWebApi : Https
{
	SlackResponse m_response;

public:
	SlackWebApi(std::wstring_view token) : Https(L"slack.com")
	{
		SetBearerToken(token);
		AddContentType(L"application/x-www-form-urlencoded");
	}

	SlackResponse & Response()
	{
		return m_response;
	}

	bool Get(const wchar_t * path)
	{
		auto response = Https::Get(path);

#if defined(_DEBUG)
		::OutputDebugStringW(L"=== Response ===\r\n");
		::OutputDebugStringW(response.Headers().c_str());
#endif
		auto contentType = response.Type();
		auto contentData = response.Data();

		if (contentType.starts_with(L"application/json"))
		{
			auto text = ConvertFrom(contentData);
			auto json = Json(text);

#if defined(_DEBUG)
			::OutputDebugStringW(text.c_str());
			::OutputDebugStringW(L"\r\n");
#endif
			VisitJson(m_response, json);

			if (m_response.ok.has_value())
			{
				return m_response.ok.value();
			}
		}

		// TODO: 例外に適切な情報を含める

		throw std::runtime_error("Unexpected response.");
	}

	bool Post(const wchar_t * path, std::wstring_view form)
	{
		auto content = ConvertFrom(form);
		auto response = Https::Post(path, content.data(), (uint32_t) content.size());

#if defined(_DEBUG)
		::OutputDebugStringW(L"=== Response ===\r\n");
		::OutputDebugStringW(response.Headers().c_str());
#endif
		auto contentType = response.Type();
		auto contentData = response.Data();

		if (contentType.starts_with(L"application/json"))
		{
			auto text = ConvertFrom(contentData);
			auto json = Json(text);

#if defined(_DEBUG)
			::OutputDebugStringW(text.c_str());
			::OutputDebugStringW(L"\r\n");
#endif
			VisitJson(m_response, json);

			if (m_response.ok.has_value())
			{
				return m_response.ok.value();
			}
		}

		// TODO: 例外に適切な情報を含める

		throw std::runtime_error("Unexpected response.");
	}
};


Custard::Custard(std::wstring_view token) : m_webapi(std::make_unique<SlackWebApi>(token)), m_token(token)
{}

Custard::~Custard() noexcept
{}

bool Custard::Post(std::wstring_view channel, std::wstring_view message)
{
	return m_webapi->Post(L"/api/chat.postMessage", std::format(L"channel={}&text={}", channel, message));
}

std::wstring Custard::BotName()
{
	return m_webapi->Response().bot.name;
}

std::filesystem::path Custard::BotIcon()
{
	FileDownloader fileDownloader(m_webapi->Response().bot.icons.image_48);
	fileDownloader.SetBearerToken(m_token);

	// TODO: ファイルが更新されているかどうかを確認する

	return fileDownloader.SaveTo(std::filesystem::temp_directory_path());
}
