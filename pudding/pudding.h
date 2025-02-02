#pragma once

#include <Windows.h>
#include <wtsapi32.h>
#include <shellapi.h>
#include <system_error>

#include "dwatch.h"
#include "profile.h"

class PopupMenu
{
	HMENU m_handle;

public:
	PopupMenu() : m_handle(::CreatePopupMenu())
	{
		if (!m_handle)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "CreatePopupMenu()");
		}
	}

	~PopupMenu() noexcept
	{
		::DestroyMenu(m_handle);
	}

	void AddItem(UINT id, const wchar_t * szText) const
	{
		if (!::AppendMenuW(m_handle, MF_STRING, id, szText))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "AppendMenuW()");
		}
	}

	void Popup(HWND hWnd, POINT pt) const
	{
		if (!::TrackPopupMenu(m_handle, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "TrackPopupMenu()");
		}
	}
};


class NotifyIcon : NOTIFYICONDATA, public PopupMenu
{
public:
	NotifyIcon(HWND hWnd, UINT uMsg, UINT uID) noexcept :
		NOTIFYICONDATA{ .cbSize = sizeof(NOTIFYICONDATA), .hWnd = hWnd, .uID = uID, .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP, .uCallbackMessage = uMsg }
	{}

	~NotifyIcon() noexcept
	{
		::Shell_NotifyIconW(NIM_DELETE, this); // !!!
	}

	void Show(HICON hIcon, const wchar_t * szTip)
	{
		this->hIcon = hIcon;

		wcscpy_s(this->szTip, szTip);

		if (!::Shell_NotifyIconW(NIM_ADD, this))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "Shell_NotifyIconW()");
		}
	}

	void Hide()
	{
		if (!::Shell_NotifyIconW(NIM_DELETE, this))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "Shell_NotifyIconW()");
		}
	}
};


class CurrentSessionInformation
{
	void * m_buff;
	DWORD m_size;

public:
	CurrentSessionInformation(const CurrentSessionInformation &) = delete;

	CurrentSessionInformation & operator=(const CurrentSessionInformation &) = delete;

	CurrentSessionInformation(WTS_INFO_CLASS infoClass) : m_buff(nullptr), m_size(0)
	{
		if (!::WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, infoClass, (LPWSTR *) &m_buff, &m_size))
		{
			throw std::system_error(::GetLastError(), std::system_category(), "WTSQuerySessionInformation()");
		}
	}

	~CurrentSessionInformation() noexcept
	{
		::WTSFreeMemory(m_buff); // !!!
	}

	operator unsigned long() const
	{
		return *(unsigned long *) m_buff;
	}

	operator unsigned short() const
	{
		return *(unsigned short *) m_buff;
	}

	const WTSCLIENTW & GetClient() const
	{
		return *(WTSCLIENTW *) m_buff;
	}
};


class CurrentSession : CurrentSessionInformation
{
	unsigned long m_sessionId;

public:
	CurrentSession() : CurrentSessionInformation(WTSClientInfo), m_sessionId(CurrentSessionInformation(WTSSessionId))
	{}

	~CurrentSession() noexcept
	{}

	unsigned long SessionId() const noexcept
	{
		return m_sessionId;
	}

	unsigned short ProtocolType() const noexcept
	{
		return CurrentSessionInformation(WTSClientProtocolType);
	}

	const wchar_t * ClientHostName() const noexcept
	{
		return GetClient().ClientName;
	}

	const wchar_t * ClientUserName() const noexcept
	{
		return GetClient().UserName;
	}
};


class PuddingWindow
{
	NotifyIcon m_trayIcon1;
	CurrentSession m_session;
	DirectoryWatcher m_watcher;

	std::wstring m_profileName;
	std::unique_ptr<Profile> m_profileData;

public:
	PuddingWindow(HWND hWnd);
	~PuddingWindow() noexcept = default;

	static LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	LRESULT OnDestroy(HWND hWnd) noexcept;
	LRESULT OnCommand(HWND hWnd, WORD wID, WORD wCode, HWND hWndControl);
	LRESULT OnTrayIcon(HWND hWnd, UINT, DWORD dwID, DWORD dwMsg);
	LRESULT OnSession(HWND hWnd, UINT, DWORD dwCode, DWORD dwID);
	LRESULT OnReload(HWND hWnd);

	void WatchSession(const wchar_t * szCode, DWORD dwCode);
	void WatchProfile(std::wstring_view file, FileAction action);
};

inline const wchar_t * WTSSESSION_CHANGE(DWORD dwCode)
{
	switch (dwCode)
	{
	case WTS_CONSOLE_CONNECT:
		return L"WTS_CONSOLE_CONNECT";

	case WTS_CONSOLE_DISCONNECT:
		return L"WTS_CONSOLE_DISCONNECT";

	case WTS_REMOTE_CONNECT:
		return L"WTS_REMOTE_CONNECT";

	case WTS_REMOTE_DISCONNECT:
		return L"WTS_REMOTE_DISCONNECT";

	case WTS_SESSION_LOGON:
		return L"WTS_SESSION_LOGON";

	case WTS_SESSION_LOGOFF:
		return L"WTS_SESSION_LOGOFF";

	case WTS_SESSION_LOCK:
		return L"WTS_SESSION_LOCK";

	case WTS_SESSION_UNLOCK:
		return L"WTS_SESSION_UNLOCK";

	case WTS_SESSION_REMOTE_CONTROL:
		return L"WTS_SESSION_REMOTE_CONTROL";

	default:
		return L"";
	}
}
