#include "previewapppanel.h"
#include "keyslideswitch.h"
#include "keycardwidget.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QLayout>
#include <QLabel>

PreviewAppPanel::PreviewAppPanel(QSystemTrayIcon* trayIcon /*= nullptr*/, QWidget* parent /*= nullptr*/)
	: QWidget(parent), m_trayIcon(trayIcon)
{
	setWindowTitle("Preview All");
	if (m_trayIcon)
		setWindowIcon(m_trayIcon->icon());
	resize(480, 320);

	m_mainLayout = new QVBoxLayout(this);
	addSwitchCard(tr("Text Files"), tr("Set Preview All as the default application for .txt files."), { ".txt" }, m_textSwitch);
	addSwitchCard(tr("Image Files"), tr("Set Preview All as the default application for .md files."), { ".png" }, m_imageSwitch);
	addSwitchCard(tr("Video Files"), tr("Set Preview All as the default application for .log files."), { ".mp4" }, m_videoSwitch);

}

PreviewAppPanel::~PreviewAppPanel()
{
}

void PreviewAppPanel::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
}

void PreviewAppPanel::onActivatedTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
	{
		show();
		activateWindow();
	}
}

void PreviewAppPanel::closeEvent(QCloseEvent* event)
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

void PreviewAppPanel::addSwitchCard(const QString& title, const QString& text, const QStringList& extList, KeySlideSwitch* switchControl)
{
	KeyCardWidget* card = new KeyCardWidget(this);
	QGridLayout* cardLayout = new QGridLayout(card);

	QLabel* titleLabel = new QLabel(title, card);
	titleLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
	QLabel* textLabel = new QLabel(text, card);
	textLabel->setStyleSheet("font-size: 14px;");
	switchControl = new KeySlideSwitch(card);
	switchControl->setFixedSize(60, 30);
	connect(switchControl, &KeySlideSwitch::toggled, this, [&extList](bool checked) {
		// ToDo: 处理扩展名注册/注销逻辑
	});

	cardLayout->addWidget(titleLabel, 0, 0);
	cardLayout->addWidget(textLabel, 1, 0);
	cardLayout->addWidget(switchControl, 0, 1, 2, 1);
	if (m_mainLayout)
		m_mainLayout->addWidget(card);
}
