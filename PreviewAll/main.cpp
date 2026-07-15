#include "previewallapplication.h"
#include "previewallmenu.h"
#include "previewallregister.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <cstdlib>

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

	PreviewAllApplication app(argc, argv);

	if (app.arguments().contains(PreviewAllRegister::REGISTER_HANDLER_ARGUMENT))
	{
		return PreviewAllRegister::registerHandler() ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	if (!PreviewAllRegister::ensureHandlerRegistered())
		return EXIT_FAILURE;

	if (!isSingleInstance())
		return 0;

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

	return 0;
}