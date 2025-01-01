#pragma once

#include <Windows.h>
#include <shellapi.h>
#include <system_error>


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

	void AddMenuItem(UINT id, const wchar_t * szText) const
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


class PuddingWindow
{
	static const UINT WM_TRAYICON = (WM_USER + 1);
	static const UINT ID_TRAYICON1 = 1001;

	NotifyIcon m_notifyIcon1;

public:
	PuddingWindow(HWND hWnd, CREATESTRUCT * ps);
	~PuddingWindow() noexcept = default;

	static LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	LRESULT OnCommand(HWND hWnd, WORD wID, WORD wCode, HWND hWndControl);
	LRESULT OnTrayIcon(HWND hWnd, UINT /*dummy*/, DWORD dwID, DWORD dwMsg);
};
