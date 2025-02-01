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


LRESULT PuddingWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// * �쐬�����C���X�^���X���P�����ł��邱�Ƃ�O��Ƃ���
	// * ����̃��b�Z�[�W�̂݁A�����o�֐��̃n���h�����Ăї�O����������

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
	case WM_TRAYICON:
		try
		{
			switch (uMsg)
			{
			case WM_COMMAND:
				return mainWindow->OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND) lParam);

			case WM_WTSSESSION_CHANGE:
				return mainWindow->OnSession(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);

			case WM_TRAYICON:
				return mainWindow->OnTrayIcon(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);
			}
		}
		catch (const std::system_error & e)
		{
			::OutputDebugStringW(MessageResource(ERROR_SYS_EXCEPTION, e.what(), e.code().value()));
		}
		catch (const std::exception & e)
		{
			::OutputDebugStringW(MessageResource(ERROR_STD_EXCEPTION, e.what()));
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

	m_watcher = DirectoryWatcher([this](std::wstring_view name, FileAction action) { WatchUpdate(name, action); }, profilePath.c_str());
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

void PuddingWindow::WatchUpdate(std::wstring_view name, FileAction action)
{
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

	if (action == Added || action == Modified || action == RenamedNewName)
	{
		if (CompareFileName(name, m_profileName))
		{
			m_profileData = LoadProfile(m_profileName);
		}
	}
}


namespace
{
	HWND CreateMainWindow(HINSTANCE hInstance)
	{
		// WNDCLASSEXW �\���̂̃����o�[�������ݒ肹���ɁA��\���Ń��b�Z�[�W������p�̃E�B���h�E���쐬����

		WindowClass<VS_TARGETNAMEW> windowClass{ hInstance, &PuddingWindow::WindowProc };

		return windowClass.NewInstance(HWND_MESSAGE);
	}
}


const MainWindow & MainWindow::GetInstance(HINSTANCE hInstance, const wchar_t * /*szCmdLine*/)
{
	static MainWindow mainWindow{ CreateMainWindow(hInstance) };
	return mainWindow;
}
