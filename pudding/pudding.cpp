#include "pudding.h"
#include "winmain.h"


PuddingWindow::PuddingWindow(HWND hWnd, CREATESTRUCT * ps) : m_notifyIcon1(hWnd, WM_TRAYICON, ID_TRAYICON1)
{
	// TODO: ����������\�[�X����ǂݍ���

	m_notifyIcon1.AddMenuItem(IDCLOSE, L"Exit");
	m_notifyIcon1.AddToTaskTray(::LoadIconW(nullptr, IDI_APPLICATION), L"Pudding");
}

LRESULT PuddingWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// * �쐬�����C���X�^���X���P�����ł��邱�Ƃ�O��Ƃ���
	// * ����̃��b�Z�[�W�����̏ꍇ�̂݁A��O���n���h�����O����

	static PuddingWindow * mainWindow;

	switch (uMsg)
	{
	case WM_CREATE:
		mainWindow = new PuddingWindow(hWnd, (CREATESTRUCT *) lParam);
		return true;

	case WM_DESTROY:
		::PostQuitMessage(0);
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
			::OutputDebugStringA(e.what());
		}
		break;

	case WM_TRAYICON:
		try
		{
			return mainWindow->OnTrayIcon(hWnd, uMsg, (DWORD) wParam, (DWORD) lParam);
		}
		catch (const std::exception & e)
		{
			::OutputDebugStringA(e.what());
		}
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
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

LRESULT PuddingWindow::OnTrayIcon(HWND hWnd, UINT /*dummy*/, DWORD dwID, DWORD dwMsg)
{
	switch (dwID)
	{
	case ID_TRAYICON1:
		if (dwMsg == WM_RBUTTONUP)
		{
			m_notifyIcon1.ShowPopupMenu(hWnd, GetCursorPos());
		}
		break;
	}

	return 0;
}


namespace
{
	HWND CreateMainWindow(HINSTANCE hInstance)
	{
		WindowClass<VS_TARGETNAME> windowClass{ hInstance, &PuddingWindow::WindowProc };

		// WNDCLASSEXW �\���̂̃����o�[�������ݒ肹���ɁA��\���Ń��b�Z�[�W������p�̃E�B���h�E���쐬����

		return windowClass.NewInstance(HWND_MESSAGE);
	}
}


const MainWindow & MainWindow::GetInstance(HINSTANCE hInstance, const wchar_t * /*szCmdLine*/)
{
	static MainWindow mainWindow{ CreateMainWindow(hInstance) };
	return mainWindow;
}
