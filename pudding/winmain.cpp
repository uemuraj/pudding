#include "winmain.h"
#include <locale>

#include <stdio.h>
#include <stdarg.h>


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /**/, _In_ LPWSTR szCmdLine, _In_ int nCmdShow)
{
	try
	{
		std::locale::global(std::locale(""));

		decltype(auto) main = MainWindow::GetInstance(hInstance, szCmdLine);

		::ShowWindow(main, nCmdShow);
		::UpdateWindow(main);

		MSG msg{};

		while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}

		return 0;
	}
	catch (const std::exception & e)
	{
		::MessageBoxA(nullptr, e.what(), VS_TARGETNAMEA, MB_ICONERROR);

		return 1;
	}
}


// https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
extern "C" IMAGE_DOS_HEADER __ImageBase;

HINSTANCE GetInstance()
{
	return (HINSTANCE) &__ImageBase;
}

MessageResource::MessageResource(LONG id, ...)
{
	va_list args{};
	va_start(args, id);
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, GetInstance(), id, 0, (LPWSTR) &m_data, 0, &args);
	va_end(args);
}

MessageResource::~MessageResource()
{
	::LocalFree(m_data);
}
