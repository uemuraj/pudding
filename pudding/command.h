#pragma once

#include <Windows.h>
#include <ranges>
#include <string>

class CommandLine
{
	int m_argc;
	wchar_t ** m_argv;

public:
	CommandLine(const wchar_t * command);
	~CommandLine() noexcept;

	CommandLine(CommandLine && other) noexcept;
	CommandLine & operator=(CommandLine && other) noexcept;

	CommandLine(const CommandLine &) = delete;
	CommandLine & operator=(const CommandLine &) = delete;

	bool operator==(const CommandLine & other) const noexcept;

	void Reset() noexcept;

	const wchar_t * File() const noexcept
	{
		return (m_argc > 0) ? m_argv[0] : L"";
	}

	auto Parameters() const noexcept
	{
		if (m_argc > 0)
		{
			return std::ranges::subrange(m_argv + 1, m_argv + m_argc);
		}
		else
		{
			return std::ranges::subrange(m_argv, m_argv);
		}
	}

	DWORD Execute(const wchar_t * directory = nullptr, int show = SW_SHOWNORMAL);
};

std::wstring EscapedParameters(const CommandLine & commandLine);
