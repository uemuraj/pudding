#pragma once

#include <Windows.h>
#include <string>
#include <utility>


HINSTANCE GetInstance();

class MessageResource
{
	wchar_t * m_data;

public:
	MessageResource(LONG id, ...) noexcept;

	~MessageResource() noexcept
	{
		::LocalFree(m_data);
	}

	operator const wchar_t * () const noexcept
	{
		return m_data;
	}

	MessageResource(MessageResource && other) noexcept : m_data(nullptr)
	{
		std::swap(m_data, other.m_data);
	}

	MessageResource(const MessageResource & other) noexcept = delete;

	MessageResource & operator=(MessageResource && other) noexcept = delete;

	MessageResource & operator=(const MessageResource & other) noexcept = delete;
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

inline std::wstring & ReplaceExtension(std::wstring & fileName, const wchar_t * ext)
{
	if (auto pos = fileName.rfind(L'.'); pos != std::wstring::npos)
	{
		fileName.resize(pos);
	}

	fileName.append(ext);

	return fileName;
}

inline std::wstring GetParentPath(const std::wstring & fileName)
{
	if (auto pos = fileName.rfind(L'\\'); pos != std::wstring::npos)
	{
		return { fileName.data(), pos ? pos : 1 };
	}

	return {};
}

bool CompareFileName(std::wstring_view a, std::wstring_view b);
