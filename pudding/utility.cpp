#include "utility.h"
#include <system_error>


// https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
extern "C" IMAGE_DOS_HEADER __ImageBase;

HINSTANCE GetInstance()
{
	return (HINSTANCE) &__ImageBase;
}

MessageResource::MessageResource(LONG id, ...) noexcept
{
	va_list args{};
	va_start(args, id);

	if (::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetInstance(), id, 0, (LPWSTR) &m_data, 0, &args) == 0)
	{
		m_data = ::StrDupW(L"null");
	}

	va_end(args);
}

MessageResource & MessageResource::operator=(const MessageResource & other) noexcept
{
	if (this != &other)
	{
		::LocalFree(m_data);

		m_data = ::StrDupW(other.m_data);
	}

	return *this;
}

GetCurrentUserName::GetCurrentUserName() : m_data{}
{
	DWORD size = __crt_countof(m_data);

	if (!::GetUserNameW(m_data, &size))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "GetUserNameW()");
	}
}

GetCurrentModuleFileName::GetCurrentModuleFileName() : std::wstring(63, L'\0')
{
	for (;;)
	{
		::SetLastError(ERROR_SUCCESS);

		auto count = ::GetModuleFileNameW(nullptr, data(), static_cast<DWORD>(size() + 1));

		auto error = ::GetLastError();

		if (error == ERROR_INSUFFICIENT_BUFFER)
		{
			resize((size_t) count + MAX_PATH);
			continue;
		}

		if (error == ERROR_SUCCESS)
		{
			resize(count);
			return;
		}

		throw std::system_error(error, std::system_category(), "GetModuleFileNameW()");
	}
}
