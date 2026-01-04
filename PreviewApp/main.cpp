#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "previewapppanel.h"

int main(int argc, char* argv[])
{
	// ToDo:单个实例

	QApplication app(argc, argv);

	// 创建托盘图标
	QSystemTrayIcon* trayIcon = new QSystemTrayIcon(QIcon(":/icons/previewall.svg"), qApp);
	trayIcon->setToolTip("Preview All");

	// 创建托盘右键菜单
	QMenu* trayMenu = new QMenu();
	QAction* showAction = trayMenu->addAction(QObject::tr("Show Preview All"));
	QAction* exitAction = trayMenu->addAction(QObject::tr("Exit"));
	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();

	// 创建主面板并连接信号槽
	PreviewAppPanel appPanel(trayIcon);
	QObject::connect(trayIcon, &QSystemTrayIcon::activated, &appPanel, &PreviewAppPanel::onActivatedTrayIcon);
	QObject::connect(showAction, &QAction::triggered, &appPanel, &PreviewAppPanel::windowToTop);
	QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
	appPanel.show();

	return app.exec();
}