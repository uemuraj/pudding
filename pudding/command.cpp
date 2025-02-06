#include "command.h"

#include <shellapi.h>
#include <shlwapi.h>

#include <algorithm>
#include <memory>
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


class ProcessHandle
{
	HANDLE m_handle;

public:
	ProcessHandle(HANDLE handle) noexcept : m_handle(handle)
	{}

	~ProcessHandle() noexcept
	{
		if (m_handle)
		{
			::CloseHandle(m_handle);
		}
	}

	operator HANDLE() const noexcept
	{
		return m_handle;
	}
};

struct ExecuteContext
{
	CommandLine commandLine;
	ExecuteCallback callback;
	ProcessHandle process;

	void GetExitCode() const noexcept
	{
		DWORD exitCode{};

		try
		{
			if (::WaitForSingleObject(process, INFINITE) != WAIT_OBJECT_0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WaitForSingleObject()");
			}

			if (!::GetExitCodeProcess(process, &exitCode))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "GetExitCodeProcess()");
			}

			callback(commandLine, exitCode, {});
		}
		catch (...)
		{
			callback(commandLine, exitCode, std::current_exception());
		}
	}
};

static void NTAPI ExecuteSimpleCallback(PTP_CALLBACK_INSTANCE, void * context)
{
	std::unique_ptr<ExecuteContext>((ExecuteContext *) context)->GetExitCode();
}


void ExecuteCommand(ExecuteCallback callback, CommandLine && commandLine, const wchar_t * directory, int show)
{
	auto parameters = EscapedParameters(commandLine);

	SHELLEXECUTEINFOW info
	{
		.cbSize = sizeof(info),
		.fMask = SEE_MASK_NOCLOSEPROCESS,
		.lpFile = commandLine.File(),
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
		return callback(commandLine, 0, {});
	}

	auto context = std::make_unique<ExecuteContext>(std::move(commandLine), callback, info.hProcess);

	if (!::TrySubmitThreadpoolCallback(ExecuteSimpleCallback, context.get(), nullptr))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "TrySubmitThreadpoolCallback()");
	}

	context.release();
}
