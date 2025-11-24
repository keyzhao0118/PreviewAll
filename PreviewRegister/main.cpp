#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include "registermainpanel.h"

int main(int argc, char* argv[])
{
	// ToDo:单个实例

	QApplication app(argc, argv);

	// 创建托盘图标
	QSystemTrayIcon* trayIcon = new QSystemTrayIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon), qApp);
	trayIcon->setToolTip("Preview All");

	// 创建托盘右键菜单
	QMenu* trayMenu = new QMenu();
	QAction* showAction = trayMenu->addAction(QObject::tr("Show Preview All"));
	QAction* exitAction = trayMenu->addAction(QObject::tr("Exit"));
	trayIcon->setContextMenu(trayMenu);
	trayIcon->show();

	// 创建主面板并连接信号槽
	RegisterMainPanel mainPanel(trayIcon);
	QObject::connect(trayIcon, &QSystemTrayIcon::activated, &mainPanel, &RegisterMainPanel::onActivatedTrayIcon);
	QObject::connect(showAction, &QAction::triggered, &mainPanel, &RegisterMainPanel::show);
	QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
	mainPanel.show();

	return app.exec();
}