#include "winmain.h"

const MainService & MainService::GetInstance()
{
	static MainService mainService;
	return mainService;
}

//// サービスメイン関数の例
//void WINAPI ServiceMain(DWORD /*argc*/, LPWSTR* /*argv*/)
//{
//	g_ServiceStatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
//
//	// サービス開始処理
//	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
//	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
//	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
//	SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//
//	// pudding のセッション監視処理をここに実装
//	// 例: メインループ、イベント監視など
//
//	// サービス停止時の処理
//	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
//	SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//}
//
//// サービスコントロールハンドラの例
//void WINAPI ServiceCtrlHandler(DWORD ctrlCode)
//{
//	if (ctrlCode == SERVICE_CONTROL_STOP) {
//		// サービス停止処理
//		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
//		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//
//		// 停止処理を実装
//
//		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
//		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
//	}
//}
