#include "pudding.h"
#include "winmain.h"
#include "utility.h"
#include "command.h"
#include "resource.h"
#include "messages.h"

constexpr UINT WM_RELOAD = (WM_USER + 1);
constexpr UINT WM_TRAYICON = (WM_USER + 2);
constexpr UINT ID_TRAYICON1 = 1001;


static MessageResource CreateStatusMessage(const CurrentSession & session)
{
	switch (session.ProtocolType())
	{
	case 0:
		return MessageResource(ID_STATUS_CONSOLE, (const wchar_t *) GetCurrentUserName());

	case 2:
		if (auto userName = session.ClientUserName(); *userName != L'\0')
		{
			return MessageResource(ID_STATUS_REMOTE_USER, userName);
		}
		if (auto hostName = session.ClientHostName(); *hostName != L'\0')
		{
			return MessageResource(ID_STATUS_REMOTE_HOST, hostName);
		}
	}

	throw std::runtime_error(__FUNCTION__ ": unknown protocol type");
}

void OutputLog(const std::system_error & e)
{
	if (::IsDebuggerPresent())
	{
		::OutputDebugStringW(MessageResource(ERROR_SYS_EXCEPTION, e.what(), e.code().value()));
	}

	// TDOD: ログファイルにも出力するようにする
}

void OutputLog(const std::exception & e)
{
	if (::IsDebuggerPresent())
	{
		::OutputDebugStringW(MessageResource(ERROR_STD_EXCEPTION, e.what()));
	}

	// TDOD: ログファイルにも出力するようにする
}

void OutputLog(LONG id, ...)
{
	va_list args{};
	va_start(args, id);

	if (::IsDebuggerPresent())
	{
		::OutputDebugStringW(MessageResource(id, args));
	}

	// TDOD: ログファイルにも出力するようにする
}


LRESULT PuddingWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// * 作成されるインスタンスが１つだけであることを前提とする
	// * 特定のメッセージのみ、メンバ関数のハンドラを呼び例外を処理する

	static PuddingWindow * mainWindow;

	switch (uMsg)
	{
	case WM_CREATE:
		mainWindow = new PuddingWindow(hWnd);
		return true;

	case WM_DESTROY:
		mainWindow->OnDestroy(hWnd);
		return 0;

	case WM_NCDESTROY:
		delete mainWindow;
		return 0;

	case WM_COMMAND:
	case WM_WTSSESSION_CHANGE:
	case WM_RELOAD:
	case WM_TRAYICON:
		try
		{
			switch (uMsg)
			{
			case WM_COMMAND:
				return mainWindow->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND) lParam);

			case WM_WTSSESSION_CHANGE:
				return mainWindow->OnSession(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);

			case WM_RELOAD:
				return mainWindow->OnReload(hWnd);

			case WM_TRAYICON:
				return mainWindow->OnTrayIcon(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);
			}
		}
		catch (const std::system_error & e)
		{
			OutputLog(e);
		}
		catch (const std::exception & e)
		{
			OutputLog(e);
		}
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

PuddingWindow::PuddingWindow(HWND hWnd) : m_trayIcon1(hWnd, WM_TRAYICON, ID_TRAYICON1)
{
	m_trayIcon1.AddItem(IDCLOSE, MessageResource(ID_MENU_EXIT, VS_TARGETNAMEW));
	m_trayIcon1.Show(::LoadIconW(nullptr, IDI_INFORMATION), CreateStatusMessage(m_session));

	if (!::WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION))
	{
		throw std::system_error(::GetLastError(), std::system_category(), "WTSRegisterSessionNotification()");
	}

	auto fileName = GetCurrentModuleFileName();
	auto profileName = ReplaceExtension(fileName, L".ini");
	auto profilePath = GetParentPath(fileName);

	m_profileName = std::move(profileName);
	m_profileData = LoadProfile(m_profileName);

	m_watcher = DirectoryWatcher([this](std::wstring_view name, FileAction action) { WatchProfile(name, action); }, profilePath.c_str());
}

LRESULT PuddingWindow::OnDestroy(HWND hWnd) noexcept
{
	::WTSUnRegisterSessionNotification(hWnd);

	::PostQuitMessage(0);

	if (auto exception = m_watcher.GetException())
	{
		try
		{
			std::rethrow_exception(exception);
		}
		catch (const std::system_error & e)
		{
			OutputLog(e);
		}
		catch (const std::exception & e)
		{
			OutputLog(e);
		}
	}

	return 0;
}

LRESULT PuddingWindow::OnCommand(HWND hWnd, WORD wID, WORD wCode, HWND hWndControl)
{
	switch (wID)
	{
	case IDCLOSE:
		::DestroyWindow(hWnd);
		break;
	}

	return 0;
}

LRESULT PuddingWindow::OnTrayIcon(HWND hWnd, UINT, DWORD dwID, DWORD dwMsg)
{
	switch (dwID)
	{
	case ID_TRAYICON1:
		if (dwMsg == WM_RBUTTONUP)
		{
			m_trayIcon1.Popup(hWnd, GetCurrentCursorPos());
		}
		break;
	}

	return 0;
}

LRESULT PuddingWindow::OnSession(HWND hWnd, UINT, DWORD dwCode, DWORD dwID)
{
	if (dwID == m_session.SessionId())
	{
		WatchSession(WTSSESSION_CHANGE(dwCode), dwCode);
	}
#if defined(_DEBUG)
	::OutputDebugStringW(MessageResource(ID_SESSION_MSG, WTSSESSION_CHANGE(dwCode), dwCode, dwID));
#endif
	return 0;
}

LRESULT PuddingWindow::OnReload(HWND hWnd)
{
	m_profileData = LoadProfile(m_profileName);

	return 0;
}

void PuddingWindow::WatchProfile(std::wstring_view name, FileAction action)
{
	if (action == Added || action == Modified || action == RenamedNewName)
	{
		if (CompareFileName(name, m_profileName))
		{
			::PostMessageW(MainWindow::GetInstance(), WM_RELOAD, 0, 0);
		}
	}
#if defined(_DEBUG)
	switch (action)
	{
	case Added:
		::OutputDebugStringW(MessageResource(ID_WATCH_ADD, name.size(), name.data()));
		break;
	case Removed:
		::OutputDebugStringW(MessageResource(ID_WATCH_REMOVE, name.size(), name.data()));
		break;
	case Modified:
		::OutputDebugStringW(MessageResource(ID_WATCH_MODIFY, name.size(), name.data()));
		break;
	case RenamedOldName:
		::OutputDebugStringW(MessageResource(ID_WATCH_RENAME_OLD, name.size(), name.data()));
		break;
	case RenamedNewName:
		::OutputDebugStringW(MessageResource(ID_WATCH_RENAME_NEW, name.size(), name.data()));
		break;
	}
#endif
}

void PuddingWindow::WatchSession(const wchar_t * szCode, DWORD dwCode)
{
	auto & section = m_profileData->operator[](szCode);

	if (auto & commandLine = section[L"CommandLine"]; !commandLine.empty())
	{
		ExecuteCallback callback = [](const CommandLine & commandLine, DWORD exitCode, std::exception_ptr exception)
		{
			OutputLog(ID_COMMAND_EXIT, commandLine.ToString().c_str(), exitCode);

			if (exception)
			{
				try
				{
					std::rethrow_exception(exception);
				}
				catch (const std::system_error & e)
				{
					OutputLog(e);
				}
				catch (const std::exception & e)
				{
					OutputLog(e);
				}
			}
		};

		if (auto & workDirectory = section[L"WorkDirectory"]; !workDirectory.empty())
		{
			ExecuteCommand(callback, commandLine.c_str(), workDirectory.c_str());
		}
		else
		{
			ExecuteCommand(callback, commandLine.c_str());
		}
	}
}


namespace
{
	HWND CreateMainWindow(HINSTANCE hInstance)
	{
		// WNDCLASSEXW 構造体のメンバーを何も設定せずに、非表示でメッセージ処理専用のウィンドウを作成する

		WindowClass<VS_TARGETNAMEW> windowClass{ hInstance, &PuddingWindow::WindowProc };

		return windowClass.NewInstance(HWND_MESSAGE);
	}
}


const MainWindow & MainWindow::GetInstance(HINSTANCE hInstance, const wchar_t * /*szCmdLine*/)
{
	static MainWindow mainWindow{ CreateMainWindow(hInstance) };
	return mainWindow;
}
