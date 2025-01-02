#include "winmain.h"
#include <locale>


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
