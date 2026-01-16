#include "previewallapplication.h"
#include "previewallmenu.h"
#include "previewallregister.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QProcess>

namespace
{

	void ensureRegisterPreviewHandler()
	{
		if (PreviewAllRegister::isRegisteredHandler())
			return;

		QProcess process;
		QString appPath = QCoreApplication::applicationFilePath();
		QStringList psArgs;
		psArgs << "-Command"
			   << QString("Start-Process -FilePath \"%1\" -ArgumentList \"--register-preview-handler\" -Verb runAs -Wait").arg(appPath);
		process.start("powershell", psArgs);
		if (!process.waitForFinished(-1))
		{
			qDebug() << "Failed to launch elevated registration process.";
		}
	}

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

	QStringList args = app.arguments();
	if (args.contains("--register-preview-handler"))
	{
		PreviewAllRegister::registerHandler();
		if(PreviewAllRegister::isRegisteredHandler())
			qDebug() << "Preview handler registered successfully.";
		else
			qDebug() << "Failed to register preview handler.";
		return 0;
	}
	ensureRegisterPreviewHandler();

	if (!isSingleInstance())
		return 0;

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