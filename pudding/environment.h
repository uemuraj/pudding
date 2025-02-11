#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

std::vector<wchar_t> NewEnvironmentStrings();

const wchar_t * GetEnvironmnetValuePtr(const wchar_t * env, std::wstring_view name);

std::wstring ExpandEnvironmentValue(const wchar_t * env, std::wstring_view value);

using EnvironmentStringsView = std::unordered_map<std::wstring_view, std::wstring_view>;


class CurrentProcessEnvironment : public EnvironmentStringsView
{
	LPWCH m_env;

public:
	CurrentProcessEnvironment();
	~CurrentProcessEnvironment() noexcept;

	CurrentProcessEnvironment(const CurrentProcessEnvironment &) = delete;
	CurrentProcessEnvironment(CurrentProcessEnvironment && other) noexcept;

	CurrentProcessEnvironment & operator=(const CurrentProcessEnvironment &) = delete;
	CurrentProcessEnvironment & operator=(CurrentProcessEnvironment &&) noexcept = delete;
};


struct EnvironmentStrings : std::unordered_map<std::wstring, std::wstring>
{
	EnvironmentStrings(HKEY hKey, const wchar_t * subKey);
	~EnvironmentStrings() noexcept = default;

	EnvironmentStrings(const EnvironmentStrings &) = delete;
	EnvironmentStrings(EnvironmentStrings && other) noexcept = default;

	EnvironmentStrings & operator=(const EnvironmentStrings &) = delete;
	EnvironmentStrings & operator=(EnvironmentStrings &&) noexcept = delete;
};


class CurentUserEnvironment : public EnvironmentStringsView
{
	EnvironmentStrings m_env;

public:
	CurentUserEnvironment();
	~CurentUserEnvironment() noexcept = default;
};

class VolatileUserEnvironment : public EnvironmentStringsView
{
	EnvironmentStrings m_env;

public:
	VolatileUserEnvironment();
	~VolatileUserEnvironment() noexcept = default;
};

class CurrentSystemEnvironment : public EnvironmentStringsView
{
	EnvironmentStrings m_env;

public:
	CurrentSystemEnvironment();
	~CurrentSystemEnvironment() noexcept = default;
};
