#include <Windows.h>
#include <shellapi.h>

#include <locale>
#include "custard.h"


class Parameters
{
	int m_argc;
	wchar_t ** m_argv;

public:
	Parameters(const wchar_t * commandLine) : m_argc(0), m_argv(nullptr)
	{
		if (commandLine && *commandLine)
		{
			m_argv = ::CommandLineToArgvW(commandLine, &m_argc);

			if (!m_argv)
			{
				throw std::system_error(::GetLastError(), std::system_category(), "CommandLineToArgvW()");
			}
		}
	}

	~Parameters() noexcept
	{
		::LocalFree(m_argv);
	}

	size_t size() const noexcept
	{
		return m_argc;
	}

	const wchar_t * operator[](size_t index) const noexcept
	{
		return (index < m_argc) ? m_argv[index] : L"";
	}
};


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /**/, _In_ LPWSTR szCmdLine, _In_ int nCmdShow)
{
	try
	{
		std::locale::global(std::locale(""));

		if (Parameters args(szCmdLine); args.size() > 2)
		{
			Custard(args[0]).PostToSlack(args[1], args[2]);
		}

		return 0;
	}
	catch (const std::system_error & e)
	{
		::MessageBoxA(nullptr, e.what(), VS_TARGETNAMEA, MB_ICONERROR);
	}
	catch (const std::exception & e)
	{
		::MessageBoxA(nullptr, e.what(), VS_TARGETNAMEA, MB_ICONERROR);
	}

	return 1;
}
