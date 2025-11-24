#include "registermainpanel.h"
#include "animatedswitch.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QLayout>

RegisterMainPanel::RegisterMainPanel(QSystemTrayIcon* trayIcon /*= nullptr*/, QWidget* parent /*= nullptr*/)
	: QWidget(parent), m_trayIcon(trayIcon)
{
	setWindowTitle("Preview All");
	if (m_trayIcon)
		setWindowIcon(m_trayIcon->icon());
	resize(480, 320);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(new AnimatedSwitch(this));


}

RegisterMainPanel::~RegisterMainPanel()
{
}

void RegisterMainPanel::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
}

void RegisterMainPanel::onActivatedTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
	{
		show();
		activateWindow();
	}
}

void RegisterMainPanel::closeEvent(QCloseEvent* event)
{
	// 当托盘可用且存在托盘图标时，隐藏到托盘而不是退出
	if (m_trayIcon && m_trayIcon->isVisible())
	{
		hide();
		m_trayIcon->showMessage(windowTitle(), tr("Minimized to System Tray"), QSystemTrayIcon::Information, 2000);
		event->ignore();
	}
	else
	{
		QWidget::closeEvent(event);
	}
}