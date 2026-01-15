#include "previewallapplication.h"
#include "previewoptionpanel.h"
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

	// 创建托盘图标
	QSystemTrayIcon* trayIcon = new QSystemTrayIcon(QIcon(":/icons/previewall.svg"), qApp);
	trayIcon->setToolTip("Preview All");

	// 创建托盘右键菜单
	QMenu* trayMenu = new QMenu();
	QAction* showAction = trayMenu->addAction(QObject::tr("Option"));
	QAction* exitAction = trayMenu->addAction(QObject::tr("Exit"));
	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();

	// 创建主面板并连接信号槽
	PreviewOptionPanel appPanel(trayIcon);
	QObject::connect(trayIcon, &QSystemTrayIcon::activated, &appPanel, &PreviewOptionPanel::onActivatedTrayIcon);
	QObject::connect(showAction, &QAction::triggered, &appPanel, &PreviewOptionPanel::windowToTop);
	QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
	appPanel.show();

	return app.exec();
}