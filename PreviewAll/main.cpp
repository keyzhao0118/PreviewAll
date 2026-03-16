#include "previewallapplication.h"
#include "previewallmenu.h"
#include "previewallregister.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QProcess>

namespace
{

	bool isSingleInstance()
	{
		HANDLE hMutex = CreateMutexW(
			nullptr,
			FALSE,
			L"Global\\FreedomKey_PreviewAll_UniqueMutex"
		);

		if (!hMutex) 
			return false;

		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hMutex);
			return false;
		}

		return true;
	}

}


int main(int argc, char* argv[])
{
	if (!isSingleInstance())
		return 0;

	PreviewAllApplication app(argc, argv);
	app.setOrganizationName("FreedomKey");
	app.setApplicationName("PreviewAll");
	app.setQuitOnLastWindowClosed(false);
	app.startWindowManageService();
	app.initTranslations();

	QSystemTrayIcon* trayIcon = new QSystemTrayIcon(QIcon(":/svg/previewall.svg"), qApp);
	trayIcon->setToolTip("Preview All");
	trayIcon->setContextMenu(new PreviewAllMenu());
	trayIcon->show();

	app.exec();
	PreviewAllRegister::unregisterAllExtentions();
	// 进程退出时反注册所有文件扩展名

	return 0;
}