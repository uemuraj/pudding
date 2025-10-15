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
			EscapedParameters(commandLine);
		}
	}

	return commandLine;
}

std::wstring CommandLine::EscapedParameters() const
{
	std::wstring buf;
	EscapedParameters(buf);
	return buf;
}

void CommandLine::EscapedParameters(std::wstring & buffer) const
{
	for (auto argv : Parameters())
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
}

std::wstring SearchExecutable(const wchar_t * path, const wchar_t * file, const wchar_t * ext)
{
	DWORD len = 12;

	std::wstring buf(len, L'\0');

	for (;;)
	{
		auto ret = ::SearchPathW(path, file, ext, len + 1, buf.data(), nullptr);

		if (!ret)
		{
			if (auto error = ::GetLastError(); error != ERROR_FILE_NOT_FOUND)
			{
				throw std::system_error(error, std::system_category(), "SearchPathW()");
			}

			::OutputDebugStringW(L"SearchPathW(): file not found. ");
			::OutputDebugStringW(L"path = ");
			::OutputDebugStringW(path ? path : L"(null)");
			::OutputDebugStringW(L", file = ");
			::OutputDebugStringW(file ? file : L"(null)");
			::OutputDebugStringW(L", ext = ");
			::OutputDebugStringW(ext ? ext : L"(null)");
			::OutputDebugStringW(L"\n");

			buf.clear();
			break;
		}

		buf.resize(ret);

		if (len >= ret)
		{
			break;
		}

		len = ret;
	}

	return buf;
}


class ExecuteContext : public STARTUPINFOW, PROCESS_INFORMATION
{
	ExecuteCallback m_callback;
	CommandLine m_commandLine;
	std::vector<wchar_t> m_environment;
	std::wstring m_directory;
	std::wstring m_path; // ìWäJçœÇ›Path

public:
	ExecuteContext(ExecuteCallback callback, CommandLine && commandLine, const wchar_t * directory) noexcept :
		STARTUPINFOW{ .cb = sizeof(STARTUPINFOW) }, PROCESS_INFORMATION{},
		m_callback(callback), m_commandLine(std::move(commandLine)), m_environment(NewEnvironmentStrings())
	{
		if (directory && *directory)
		{
			m_directory = ExpandEnvironmentValue(m_environment.data(), directory);
		}
		// PathÇ‡ìWäJÇµÇƒï€éù
		const wchar_t * rawPath = GetEnvironmnetValuePtr(m_environment.data(), L"Path");
		if (rawPath)
		{
			m_path = ExpandEnvironmentValue(m_environment.data(), rawPath);
		}
	}

	~ExecuteContext() noexcept
	{
		::CloseHandle(hProcess);
		::CloseHandle(hThread);
	}

	void Execute()
	{
		auto executable = SearchExecutable();
		auto commandLine = m_commandLine.EscapedParameters();

		auto exe = executable.data();
		auto cmd = commandLine.data();
		auto env = m_environment.data();
		auto dir = GetWorkDirectory();

		constexpr DWORD flags = CREATE_UNICODE_ENVIRONMENT;

		if (!::CreateProcessW(exe, cmd, nullptr, nullptr, false, flags, env, dir, this, this))
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

private:
	std::wstring SearchExecutable()
	{
		std::wstring executable;

		auto file = m_commandLine.File();

		if (auto dir = GetWorkDirectory(); dir)
		{
			executable = ::SearchExecutable(dir, file, L".exe");
		}

		if (executable.empty())
		{
			executable = ::SearchExecutable(GetPath(), file, L".exe");
		}

		if (executable.empty())
		{
			throw std::system_error(ERROR_FILE_NOT_FOUND, std::system_category(), "SearchExecutable()");
		}

		return executable;
	}

	const wchar_t * GetPath() const noexcept
	{
		return m_path.empty() ? nullptr : m_path.c_str();
	}

	const wchar_t * GetWorkDirectory() const noexcept
	{
		return m_directory.empty() ? nullptr : m_directory.c_str();
	}
};

static void NTAPI ExecuteSimpleCallback(PTP_CALLBACK_INSTANCE, void * context)
{
	std::unique_ptr<ExecuteContext>((ExecuteContext *) context)->GetExitCode();
}

void ExecuteCommand(ExecuteCallback callback, CommandLine && commandLine, const wchar_t * directory, int show)
{
	auto context = std::make_unique<ExecuteContext>(callback, std::move(commandLine), directory);

	context->dwFlags = STARTF_USESHOWWINDOW;
	context->wShowWindow = show;
	context->Execute();

	if (!::TrySubmitThreadpoolCallback(ExecuteSimpleCallback, context.get(), nullptr))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "TrySubmitThreadpoolCallback()");
	}

	context.release();
}
