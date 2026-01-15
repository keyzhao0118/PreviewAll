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
	addSwitchCard(tr("Image Files"), PreviewAllRegister::imageExtList, &m_imageSwitch, "image");
	addSwitchCard(tr("Archive Files"), PreviewAllRegister::archiveExtList, &m_archiveSwitch, "archive");
	addSwitchCard(tr("Source Code"), PreviewAllRegister::codeExtList, &m_codeSwitch, "code");
	initCheckState();
}

PreviewOptionPanel::~PreviewOptionPanel()
{
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

void PreviewOptionPanel::addSwitchCard(const QString& title, const QStringList& extList, KeySlideSwitch** switchControl, const QString& type)
{
	KeyCardWidget* card = new KeyCardWidget(this);
	QHBoxLayout* cardLayout = new QHBoxLayout(card);

	QLabel* titleLabel = new QLabel(title, card);
	titleLabel->setStyleSheet("font-weight: bold; font-size: 18px;");

	*switchControl = new KeySlideSwitch(card);
	(*switchControl)->setFixedSize(60, 30);
	connect(*switchControl, &KeySlideSwitch::toggled, this, [=](bool checked) {
		if (checked)
			PreviewAllRegister::registerExtentions(extList);
		else
			PreviewAllRegister::unregisterExtentions(extList);
		QSettings settings;
		settings.setValue("switchState/" + type, checked);
	});

	cardLayout->addWidget(titleLabel);
	cardLayout->addWidget(*switchControl);
	
	if (m_mainLayout)
		m_mainLayout->addWidget(card);
}

void PreviewOptionPanel::initCheckState()
{
	QSettings settings;
	bool imageState = settings.value("switchState/image", false).toBool();
	bool archiveState = settings.value("switchState/archive", false).toBool();
	bool codeState = settings.value("switchState/code", false).toBool();

	if (imageState) PreviewAllRegister::registerExtentions(PreviewAllRegister::imageExtList);
	if (archiveState) PreviewAllRegister::registerExtentions(PreviewAllRegister::archiveExtList);
	if (codeState) PreviewAllRegister::registerExtentions(PreviewAllRegister::codeExtList);

	if (m_imageSwitch) m_imageSwitch->setChecked(imageState);
	if (m_archiveSwitch) m_archiveSwitch->setChecked(archiveState);
	if (m_codeSwitch) m_codeSwitch->setChecked(codeState);
}

