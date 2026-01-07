#include "previewallapplication.h"
#include "previewoptionpanel.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>


int main(int argc, char* argv[])
{
	// ToDo:单个实例

	PreviewAllApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	app.startWindowManageService();

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