#include "pudding.h"
#include "winmain.h"
#include "utility.h"

#include "resource.h"
#include "messages.h"

constexpr UINT WM_TRAYICON = (WM_USER + 1);
constexpr UINT ID_TRAYICON1 = 1001;


static MessageResource CreateStatusMessage(const CurrentSession & session)
{
	switch (session.ProtocolType())
	{
	case 0:
		return MessageResource(ID_STATUS_CONSOLE, (const wchar_t *) GetCurrentUserName());

	case 2:
		if (auto userName = session.ClientUserName(); *userName)
		{
			return MessageResource(ID_STATUS_REMOTE_USER, userName);
		}
		if (auto hostName = session.ClientHostName(); *hostName)
		{
			return MessageResource(ID_STATUS_REMOTE_HOST, hostName);
		}
	}

	throw std::runtime_error(__FUNCTION__ ": unknown protocol type");
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
		try
		{
			return mainWindow->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND) lParam);
		}
		catch (const std::exception & e)
		{
			::OutputDebugStringW(MessageResource(ERROR_EXCEPTION, e.what()));
		}
		break;

	case WM_WTSSESSION_CHANGE:
		try
		{
			return mainWindow->OnSession(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);
		}
		catch (const std::exception & e)
		{
			::OutputDebugStringW(MessageResource(ERROR_EXCEPTION, e.what()));
		}
		break;

	case WM_TRAYICON:
		try
		{
			return mainWindow->OnTrayIcon(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);
		}
		catch (const std::exception & e)
		{
			::OutputDebugStringW(MessageResource(ERROR_EXCEPTION, e.what()));
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
}

LRESULT PuddingWindow::OnDestroy(HWND hWnd) noexcept
{
	::WTSUnRegisterSessionNotification(hWnd);
	::PostQuitMessage(0);
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

LRESULT PuddingWindow::OnSession(HWND hWnd, UINT, DWORD dwCode, DWORD dwID)
{
	if (dwID == m_session.SessionId())
	{
		auto profile = LoadProfile();

		switch (dwCode)
		{
		case WTS_CONSOLE_CONNECT:
			::OutputDebugStringW(L"WTS_CONSOLE_CONNECT\r\n");
			break;
		case WTS_CONSOLE_DISCONNECT:
			::OutputDebugStringW(L"WTS_CONSOLE_DISCONNECT\r\n");
			break;
		case WTS_REMOTE_CONNECT:
			::OutputDebugStringW(L"WTS_REMOTE_CONNECT\r\n");
			break;
		case WTS_REMOTE_DISCONNECT:
			::OutputDebugStringW(L"WTS_REMOTE_DISCONNECT\r\n");
			break;
		case WTS_SESSION_LOGON:
			::OutputDebugStringW(L"WTS_SESSION_LOGON\r\n");
			break;
		case WTS_SESSION_LOGOFF:
			::OutputDebugStringW(L"WTS_SESSION_LOGOFF\r\n");
			break;
		case WTS_SESSION_LOCK:
			::OutputDebugStringW(L"WTS_SESSION_LOCK\r\n");
			break;
		case WTS_SESSION_UNLOCK:
			::OutputDebugStringW(L"WTS_SESSION_UNLOCK\r\n");
			break;
		case WTS_SESSION_REMOTE_CONTROL:
			::OutputDebugStringW(L"WTS_SESSION_REMOTE_CONTROL\r\n");
			break;
		}
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
