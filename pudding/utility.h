#pragma once

#include <Windows.h>
#include <shlwapi.h>

#include <string>
#include <utility>


HINSTANCE GetInstance();

class MessageResource
{
	wchar_t * m_data;

public:
	MessageResource(LONG id, ...) noexcept;

	operator const wchar_t * ()
	{
		return m_data;
	}

	MessageResource(MessageResource && other) noexcept : m_data(nullptr)
	{
		std::swap(m_data, other.m_data);
	}

	MessageResource & operator=(MessageResource && other) noexcept
	{
		std::swap(m_data, other.m_data);
	}

	MessageResource(const MessageResource & other) noexcept
	{
		m_data = ::StrDupW(other.m_data);
	}

	MessageResource & operator=(const MessageResource & other) noexcept;

	~MessageResource() noexcept
	{
		::LocalFree(m_data);
	}
};

class GetCurrentUserName
{
	wchar_t m_data[256 + 1]; // <lmcons.h> UNLEN 256

public:
	GetCurrentUserName();
	~GetCurrentUserName() = default;

	operator const wchar_t * () const
	{
		return m_data;
	}
};

struct GetCurrentCursorPos : POINT
{
	GetCurrentCursorPos() noexcept : POINT{}
	{
		::GetCursorPos(this);
	}
};

struct GetCurrentModuleFileName : std::wstring
{
	GetCurrentModuleFileName();
	~GetCurrentModuleFileName() = default;

	operator const wchar_t * () const
	{
		return c_str();
	}
};
