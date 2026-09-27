#include "app/previewallapplication.h"
#include "app/previewallmenu.h"
#include "app/previewallregister.h"
#include "preview/previewwidget.h"
#include <QFileInfo>
#include <QDebug>
#include <QSystemTrayIcon>
#include <cstdlib>
#include <Windows.h>

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
	app.setOrganizationName("FreedomKey");
	app.setApplicationName("PreviewAll");
	app.initTranslations();

	// Open a standalone preview for local development without COM registration.
	const QStringList arguments = app.arguments();
	const int previewIndex = arguments.indexOf("--preview");
	if (previewIndex >= 0)
	{
		if (previewIndex + 1 >= arguments.size())
			return EXIT_FAILURE;
		const QFileInfo file(arguments[previewIndex + 1]);
		if (!file.isFile())
			return EXIT_FAILURE;
		QWidget* preview = new PreviewWidget(file.absoluteFilePath());
		preview->setAttribute(Qt::WA_DeleteOnClose);
		preview->setWindowFlags(Qt::Window);
		preview->setWindowTitle("PreviewAll - " + file.fileName());
		preview->resize(960, 720);
		preview->show();
		return app.exec();
	}

	if (app.arguments().contains(PreviewAllRegister::REGISTER_HANDLER_ARGUMENT))
	{
		return PreviewAllRegister::registerHandler() ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	if (!PreviewAllRegister::ensureHandlerRegistered())
		return EXIT_FAILURE;

	if (!isSingleInstance())
		return 0;

	app.setQuitOnLastWindowClosed(false);
	app.startWindowManageService();

	QSystemTrayIcon* trayIcon = new QSystemTrayIcon(QIcon(":/svg/previewall.svg"), qApp);
	trayIcon->setToolTip("Preview All");
	trayIcon->setContextMenu(new PreviewAllMenu());
	trayIcon->show();

	app.exec();
	PreviewAllRegister::unregisterAllExtentions();

	return 0;
}
