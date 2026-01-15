#include "previewoptionpanel.h"
#include "previewallregister.h"
#include "keyslideswitch.h"
#include "keycardwidget.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QLayout>
#include <QLabel>
#include <QSettings>
#include <QDir>
#include <QApplication>

PreviewOptionPanel::PreviewOptionPanel(QSystemTrayIcon* trayIcon /*= nullptr*/, QWidget* parent /*= nullptr*/)
	: QWidget(parent), m_trayIcon(trayIcon)
{
	setWindowFlag(Qt::WindowMaximizeButtonHint, false);
	setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
	setFixedSize(320, 320);

	if (m_trayIcon)
		setWindowIcon(m_trayIcon->icon());

	m_mainLayout = new QVBoxLayout(this);
	addSwitchCard(tr("Image Files"), PreviewAllRegister::imageExtList, &m_imageSwitch);
	addSwitchCard(tr("Archive Files"), PreviewAllRegister::archiveExtList, &m_archiveSwitch);
	addSwitchCard(tr("Source Code"), PreviewAllRegister::codeExtList, &m_codeSwitch);
}

PreviewOptionPanel::~PreviewOptionPanel()
{
}

void PreviewOptionPanel::showEvent(QShowEvent* event)
{
	if (m_imageSwitch) m_imageSwitch->setChecked(getRegisterState(PreviewAllRegister::imageExtList));
	if (m_archiveSwitch) m_archiveSwitch->setChecked(getRegisterState(PreviewAllRegister::archiveExtList));
	if (m_codeSwitch) m_codeSwitch->setChecked(getRegisterState(PreviewAllRegister::codeExtList));

	QWidget::showEvent(event);
}

void PreviewOptionPanel::windowToTop()
{
	show();
	activateWindow();
}

void PreviewOptionPanel::onActivatedTrayIcon(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
		windowToTop();
}

void PreviewOptionPanel::closeEvent(QCloseEvent* event)
{
	// 当托盘可用且存在托盘图标时，隐藏到托盘而不是退出
	if (m_trayIcon && m_trayIcon->isVisible())
	{
		hide();
		event->ignore();
	}
	else
	{
		QWidget::closeEvent(event);
	}
}

void PreviewOptionPanel::addSwitchCard(const QString& title, const QStringList& extList, KeySlideSwitch** switchControl)
{
	KeyCardWidget* card = new KeyCardWidget(this);
	QHBoxLayout* cardLayout = new QHBoxLayout(card);

	QLabel* titleLabel = new QLabel(title, card);
	titleLabel->setStyleSheet("font-weight: bold; font-size: 18px;");

	*switchControl = new KeySlideSwitch(card);
	(*switchControl)->setFixedSize(60, 30);
	connect(*switchControl, &KeySlideSwitch::toggled, this, [=](bool checked) {
		for (const QString& ext : extList)
		{
			if (checked)
				PreviewAllRegister::registerExtention(ext);
			else
				PreviewAllRegister::unregisterExtention(ext);
		}
	});

	cardLayout->addWidget(titleLabel);
	cardLayout->addWidget(*switchControl);
	
	if (m_mainLayout)
		m_mainLayout->addWidget(card);
}

bool PreviewOptionPanel::getRegisterState(const QStringList& extList)
{
	if (!PreviewAllRegister::isRegisteredHandler())
		return false;

	for (const QString& ext : extList)
	{
		if (!PreviewAllRegister::isRegisteredExtention(ext))
			return false;
	}

	return true;
}
