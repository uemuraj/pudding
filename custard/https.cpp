#include "https.h"
#include <stdexcept>
#include <system_error>

using namespace custard;

Session::Session() : Handle(::WinHttpOpen(L"A WinHTTP Program Custard/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0))
{
	if (!m_handle)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpen");
	}
}

Connection::Connection(Session & session, const wchar_t * host) : Handle(::WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0))
{
	if (m_handle == nullptr)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WinHttpConnect");
	}
}

Request::Request(Connection & connection, const wchar_t * verb, const wchar_t * path) : Handle(::WinHttpOpenRequest(connection, verb, path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE))
{
	if (m_handle == nullptr)
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WinHttpOpenRequest");
	}
}

void Request::Send(const wchar_t * headers, void * content, uint32_t size)
{
	if (!::WinHttpSendRequest(m_handle, headers, -1, content, size, size, 0))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WinHttpSendRequest");
	}
}

Response::Response(Request & request) : Handle(std::exchange(request.m_handle, nullptr))
{
	if (!::WinHttpReceiveResponse(m_handle, nullptr))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReceiveResponse");
	}
}

std::wstring Response::Headers()
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

uint32_t Response::ContentLength()
{
	DWORD size = sizeof(DWORD);
	DWORD length = 0;

	if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &length, &size, nullptr))
	{
		if (auto error = ::GetLastError(); error != ERROR_WINHTTP_HEADER_NOT_FOUND)
		{
			throw std::system_error(error, std::system_category(), "WinHttpQueryHeaders");
		}
	}

	return length;
}

std::wstring Response::ContentType()
{
	DWORD size = 0;

	if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_TYPE, nullptr, nullptr, &size, nullptr))
	{
		auto error = GetLastError();

		if (error == ERROR_WINHTTP_HEADER_NOT_FOUND)
		{
			return {};
		}

		if (error != ERROR_INSUFFICIENT_BUFFER)
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

std::wstring Response::ContentEncoding()
{
	DWORD size = 0;

	if (!::WinHttpQueryHeaders(m_handle, WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING, nullptr, nullptr, &size, nullptr))
	{
		auto error = GetLastError();

		if (error == ERROR_WINHTTP_HEADER_NOT_FOUND)
		{
			return {};
		}

		if (error != ERROR_INSUFFICIENT_BUFFER)
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

static DWORD QueryDataAvailable(HINTERNET hRequest)
{
	DWORD size = 0;

	if (!::WinHttpQueryDataAvailable(hRequest, &size))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WinHttpQueryDataAvailable");
	}

	return size;
}

std::vector<std::byte> Response::GetContent()
{
	std::vector<std::byte> buff;

	if (auto length = ContentLength(); length == 0)
	{
		buff.reserve(Minimum);
	}
	else if (length <= Maximum)
	{
		buff.reserve(length);
	}
	else
	{
		throw std::logic_error("Content-Length is too large.");
	}

	size_t offset = 0;

	for (auto size = QueryDataAvailable(m_handle), read = 0ul; size > 0; size = QueryDataAvailable(m_handle), offset += read)
	{
		buff.resize(offset + size);

		if (!::WinHttpReadData(m_handle, buff.data() + offset, size, &read))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReadData");
		}

		if (read < size)
		{
			throw std::underflow_error(__FUNCTION__);
		}
	}

	return buff;
}

void Response::Recv(std::function<void(std::byte *, uint32_t)> callback)
{
	std::vector<std::byte> buff(Minimum);

	for (auto size = QueryDataAvailable(m_handle); size > 0; size = QueryDataAvailable(m_handle))
	{
		DWORD read{};

		if (!::WinHttpReadData(m_handle, buff.data(), Minimum, &read))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WinHttpReadData");
		}

		if (read < size)
		{
			throw std::underflow_error(__FUNCTION__);
		}

		callback(buff.data(), read);
	}
}

Url::Url(std::wstring_view url) :
	URL_COMPONENTS{ .dwStructSize = sizeof(URL_COMPONENTS), .dwHostNameLength = DWORD(-1), .dwUrlPathLength = DWORD(-1) }, m_url(url)
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
}

struct FileWriter
{
	HANDLE m_handle;

	FileWriter(const wchar_t * fileName) : m_handle(::CreateFileW(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr))
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

DownloadUrl::DownloadUrl(std::wstring_view url) : Url(url), Https(Host().c_str())
{
	std::wstring_view path{ lpszUrlPath, dwUrlPathLength };

	if (auto pos = path.find_last_of(L'/'); pos != std::wstring_view::npos)
	{
		m_fileName = path.substr(pos + 1);
		return;
	}

	throw std::runtime_error("Invalid URL.");
}

std::filesystem::path DownloadUrl::SaveTo(const std::filesystem::path & dest)
{
	std::error_code ec{};

	if (!std::filesystem::is_directory(dest, ec))
	{
		throw std::system_error(ec, "std::filesystem::is_directory");
	}

	auto file = dest / m_fileName;

	// TODO: タイムスタンプを使ってキャッシュする

	Response response = Get(Path().c_str());
	response.Recv(FileWriter(file.c_str()));

#if defined(_DEBUG)
	::OutputDebugStringW(L"=== Response ===\r\n");
	::OutputDebugStringW(response.Headers().c_str());
	::OutputDebugStringW(L"================\r\n");
#endif
	return file;
}

namespace custard
{
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

	std::wstring ConvertFrom(const std::vector<std::byte> & data)
	{
		return ConvertFrom({ (const char8_t *) data.data(), data.size() });
	}
}
