#include "winmain.h"

const MainService & MainService::GetInstance()
{
	static MainService mainService;
	return mainService;
}

//// �T�[�r�X���C���֐��̗�
//void WINAPI ServiceMain(DWORD /*argc*/, LPWSTR* /*argv*/)
//{
//	g_ServiceStatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
//
//	// �T�[�r�X�J�n����
//	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
//	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
//	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
//	SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//
//	// pudding �̃Z�b�V�����Ď������������Ɏ���
//	// ��: ���C�����[�v�A�C�x���g�Ď��Ȃ�
//
//	// �T�[�r�X��~���̏���
//	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
//	SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//}
//
//// �T�[�r�X�R���g���[���n���h���̗�
//void WINAPI ServiceCtrlHandler(DWORD ctrlCode)
//{
//	if (ctrlCode == SERVICE_CONTROL_STOP) {
//		// �T�[�r�X��~����
//		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
//		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//
//		// ��~����������
//
//		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
//		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//	}
//}
