#include "command.h"
#include "environment.h"

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

std::wstring CommandLine::ToString() const
{
	std::wstring commandLine;

	if (m_argc > 0)
	{
		bool quote = false;

		commandLine.push_back(L'"');

		for (const wchar_t * file = m_argv[0]; *file; ++file)
		{
			const wchar_t ch = *file;

			commandLine.push_back(ch);

			if (iswblank(ch))
			{
				quote = true;
			}
		}

		if (quote)
			commandLine.push_back(L'"');
		else
			commandLine.erase(0, 1);

		if (m_argc > 1)
		{
			EscapedParameters(commandLine, *this);
		}
	}

	return commandLine;
}

std::wstring & EscapedParameters(std::wstring & buffer, const CommandLine & commandLine)
{
	for (auto argv : commandLine.Parameters())
	{
		std::wstring_view parameter(argv);

		if (!buffer.empty())
		{
			buffer.push_back(L' ');
		}

		bool quote = parameter.find_first_of(L" \t") != std::wstring_view::npos;

		if (quote)
		{
			buffer.push_back(L'"');
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
				buffer.append(backslash * 2, L'\\');
				buffer.push_back(quote ? L'"' : L'\\');
				backslash = 0;
			}

			if (backslash > 0)
			{
				buffer.append(backslash, L'\\');
				backslash = 0;
			}

			buffer.push_back(ch);
		}

		if (backslash > 0)
		{
			buffer.append(backslash, L'\\');
		}

		if (quote)
		{
			buffer.push_back(L'"');
		}
	}

	return buffer;
}


class ExecuteContext : public STARTUPINFOW, PROCESS_INFORMATION
{
	ExecuteCallback m_callback;
	CommandLine m_commandLine;
	std::vector<wchar_t> m_environment;

public:
	ExecuteContext(ExecuteCallback callback, CommandLine && commandLine) noexcept :
		STARTUPINFOW{ .cb = sizeof(STARTUPINFOW) }, PROCESS_INFORMATION{},
		m_callback(callback), m_commandLine(std::move(commandLine)), m_environment(NewEnvironmentStrings())
	{}

	~ExecuteContext() noexcept
	{
		::CloseHandle(hProcess);
		::CloseHandle(hThread);
	}

	void Execute(const wchar_t * dir)
	{
		// TODO: m_commandLine.File() に環境変数が含まれている場合の処理を追加する
		// TODO: m_commandLine.File() から意図的に実行可能ファイルを検索して CreateProcessW 関数の第一引数とする
		// -> https://learn.microsoft.com/ja-jp/windows/win32/shell/app-registration#finding-an-application-executable
		// -> https://learn.microsoft.com/ja-jp/windows/win32/api/processenv/nf-processenv-searchpathw

		auto commandLine = m_commandLine.ToString();
		auto cmd = commandLine.data();
		auto env = m_environment.data();

		DWORD flags = CREATE_UNICODE_ENVIRONMENT;

		if (!::CreateProcessW(nullptr, cmd, nullptr, nullptr, false, flags, env, dir, this, this))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "CreateProcessW()");
		}
	}

	void GetExitCode() const noexcept
	{
		DWORD exitCode{};

		try
		{
			if (::WaitForSingleObject(hProcess, INFINITE) != WAIT_OBJECT_0)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "WaitForSingleObject()");
			}

			if (!::GetExitCodeProcess(hProcess, &exitCode))
			{
				throw std::system_error(::GetLastError(), std::system_category(), "GetExitCodeProcess()");
			}

			m_callback(m_commandLine, exitCode, {});
		}
		catch (...)
		{
			m_callback(m_commandLine, exitCode, std::current_exception());
		}
	}
};

static void NTAPI ExecuteSimpleCallback(PTP_CALLBACK_INSTANCE, void * context)
{
	std::unique_ptr<ExecuteContext>((ExecuteContext *) context)->GetExitCode();
}

void ExecuteCommand(ExecuteCallback callback, CommandLine && commandLine, const wchar_t * directory, int show)
{
	auto context = std::make_unique<ExecuteContext>(callback, std::move(commandLine));

	context->dwFlags = STARTF_USESHOWWINDOW;
	context->wShowWindow = show;
	context->Execute(directory);

	if (!::TrySubmitThreadpoolCallback(ExecuteSimpleCallback, context.get(), nullptr))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "TrySubmitThreadpoolCallback()");
	}

	context.release();
}
