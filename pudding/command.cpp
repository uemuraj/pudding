#include "command.h"

#include <shellapi.h>
#include <algorithm>
#include <utility>
#include <system_error>

CommandLine::CommandLine(const wchar_t * command) : m_argc(0), m_argv(nullptr)
{
	if (command && *command)
	{
		m_argv = ::CommandLineToArgvW(command, &m_argc);

		if (!m_argv)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "CommandLineToArgvW()");
		}
	}
}

CommandLine::~CommandLine() noexcept
{
	Reset();
}

CommandLine::CommandLine(CommandLine && other) noexcept : m_argc(0), m_argv(nullptr)
{
	std::swap(m_argc, other.m_argc);
	std::swap(m_argv, other.m_argv);
}

CommandLine & CommandLine::operator=(CommandLine && other) noexcept
{
	if (this != &other)
	{
		Reset();
		std::swap(m_argc, other.m_argc);
		std::swap(m_argv, other.m_argv);
	}

	return *this;
}

bool CommandLine::operator==(const CommandLine & other) const noexcept
{
	if (m_argc == other.m_argc)
	{
		return std::ranges::equal(m_argv, m_argv + m_argc, other.m_argv, other.m_argv + other.m_argc, [](auto a, auto b) { return wcscmp(a, b) == 0; });
	}

	return false;
}

void CommandLine::Reset() noexcept
{
	::LocalFree(m_argv);

	m_argc = 0, m_argv = nullptr;
}

DWORD CommandLine::Execute(const wchar_t * directory, int show)
{
	auto parameters = EscapedParameters(*this);

	SHELLEXECUTEINFOW info
	{
		.cbSize = sizeof(info),
		.fMask = SEE_MASK_NOCLOSEPROCESS,
		.lpFile = File(),
		.lpParameters = parameters.c_str(),
		.lpDirectory = directory,
		.nShow = show
	};

	if (!::ShellExecuteExW(&info))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "ShellExecuteExW()");
	}

	if (!info.hProcess)
	{
		return 0; // no process
	}

	try
	{
		::WaitForSingleObject(info.hProcess, INFINITE);

		DWORD exitCode{};

		if (!::GetExitCodeProcess(info.hProcess, &exitCode))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "GetExitCodeProcess()");
		}

		::CloseHandle(info.hProcess);
		return exitCode;
	}
	catch (...)
	{
		::CloseHandle(info.hProcess);
		throw;
	}
}

std::wstring EscapedParameters(const CommandLine & commandLine)
{
	std::wstring parameters;

	for (auto argv : commandLine.Parameters())
	{
		std::wstring_view parameter(argv);

		if (!parameters.empty())
		{
			parameters.push_back(L' ');
		}

		bool quote = parameter.find_first_of(L" \t") != std::wstring_view::npos;

		if (quote)
		{
			parameters.push_back(L'"');
		}

		int backslash = 0;

		for (auto ch : parameter)
		{
			if (ch == L'\\')
			{
				++backslash;
				continue;
			}

			if (ch == L'"')
			{
				parameters.append(backslash * 2, L'\\');
				parameters.push_back(quote ? L'"' : L'\\');
				backslash = 0;
			}

			if (backslash > 0)
			{
				parameters.append(backslash, L'\\');
				backslash = 0;
			}

			parameters.push_back(ch);
		}

		if (backslash > 0)
		{
			parameters.append(backslash, L'\\');
		}

		if (quote)
		{
			parameters.push_back(L'"');
		}
	}

	return parameters;
}
