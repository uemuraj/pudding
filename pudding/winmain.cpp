#include "winmain.h"
#include "utility.h"
#include <locale>


static int RunAsService()
{
	try
	{
		std::locale::global(std::locale(""));

		// TDOO: サービスとして動作するように実装する

		return 0;
	}
	catch (const std::exception & e)
	{
		// TODO: サービスとして動作する場合のエラーメッセージの出力方法を考える

		return 1;
	}
}


static int RunAsApplication(HINSTANCE hInstance, const wchar_t * szCmdLine, int nCmdShow)
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


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /**/, _In_ LPWSTR szCmdLine, _In_ int nCmdShow)
{
	if (ContainsSubstringIgnoreCase(szCmdLine, L"/service"))
	{
		return RunAsService();
	}
	else
	{
		return RunAsApplication(hInstance, szCmdLine, nCmdShow);
	}
}
