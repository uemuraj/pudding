#pragma once

#include <Windows.h>
#include <system_error>


class MainWindow
{
	HWND m_hWnd;

protected:
	MainWindow(HWND hWnd) noexcept : m_hWnd(hWnd) {}
	~MainWindow() = default;

public:
	static const MainWindow & GetInstance(HINSTANCE hInstance, const wchar_t * szCmdLine);

	operator HWND() const
	{
		return m_hWnd;
	}
};


template <std::size_t N>
struct WindowClassName
{
	wchar_t value[N];

	constexpr WindowClassName(const wchar_t(&name)[N])
	{
		// C++23 では std::copy() が使用可能

		for (std::size_t i = 0; i < N; ++i)
		{
			value[i] = name[i];
		}
	}
};


template <WindowClassName literal>
class WindowClass : WNDCLASSEXW
{
public:
	WindowClass(HINSTANCE hInstance, WNDPROC fnWndProc) :
		WNDCLASSEXW{ .cbSize = sizeof(WNDCLASSEXW), .lpfnWndProc = fnWndProc, .hInstance = hInstance, .lpszClassName = literal.value }
	{}

	~WindowClass() noexcept
	{
		// アプリケーション終了時、自動的に登録解除されるため何もしない
	}

	HWND NewInstance(HWND hWndParent, int X = CW_USEDEFAULT, int Y = 0, int nWidth = CW_USEDEFAULT, int nHeight = 0) const
	{
		static auto clazz = Register();

		auto hWnd = ::CreateWindowExW(0, LPCWSTR(clazz), nullptr, 0, X, Y, nWidth, nHeight, hWndParent, nullptr, hInstance, nullptr);

		if (!hWnd)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "CreateWindowExW()");
		}

		return hWnd;
	}

private:
	ATOM Register() const
	{
		auto atom = ::RegisterClassExW(this);

		if (!atom)
		{
			throw std::system_error(::GetLastError(), std::system_category(), "RegisterClassExW()");
		}

		return atom;
	}
};


inline POINT GetCursorPos() noexcept
{
	POINT pt;
	::GetCursorPos(&pt);
	return pt;
}


HINSTANCE GetInstance();


class MessageResource
{
	wchar_t * m_data;

public:
	MessageResource(HINSTANCE hInstance, LONG id, ...);
	MessageResource(LONG id, ...);
	~MessageResource();

	MessageResource(const MessageResource &) = delete;
	MessageResource & operator = (const MessageResource &) = delete;

	MessageResource(MessageResource &&) = default;
	MessageResource & operator = (MessageResource &&) = default;

	operator const wchar_t * ()
	{
		return m_data ? m_data : L"null";
	}
};
