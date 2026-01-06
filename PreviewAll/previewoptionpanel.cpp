#include "previewoptionpanel.h"
#include "keyslideswitch.h"
#include "keycardwidget.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QLayout>
#include <QLabel>
#include <QSettings>
#include <QDir>
#include <QApplication>

namespace
{

	const QStringList s_imageExtList = { ".png",".jpg",".jpeg",".tif",".tiff",".bmp",".webp",".ico",".svg",".gif" };
	const QStringList s_archiveExtList = {".zip", ".rar", ".7z"};
	const QStringList s_codeExtList = { ".c" };

	const QString CLSID_PreviewHandlerCategory = "{8895b1c6-b41f-4c1c-a562-0d564250836f}";
	const QString CLSID_PreviewAllHandler = "{A26D5A00-AF3F-47B7-B075-A3282DE904E6}";
	const QString APPID_PREVHOST64 = "{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}";
	const QString NAME_PreviewAllHandler = "PreviewAllHandler";

	void registerPreviewAllHandler()
	{
		{
			QSettings clsidRoot("HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
			clsidRoot.setValue(".", NAME_PreviewAllHandler);
			clsidRoot.setValue("AppID", APPID_PREVHOST64);
			clsidRoot.setValue("DisableLowILProcessIsolation", 1);

			QSettings inproc("HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler + "\\InProcServer32", QSettings::NativeFormat);
			QString handlerPath = "D:\\workspace\\PreviewAll\\out\\build\\x64-debug-user\\PreviewAllHandler\\PreviewAllHandler.dll";
			inproc.setValue(".", handlerPath);
			inproc.setValue("ThreadingModel", "Apartment");
		}

		{
			QSettings previewHandlers("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", QSettings::NativeFormat);
			previewHandlers.setValue(CLSID_PreviewAllHandler, NAME_PreviewAllHandler);
		}
	}

	void unregisterPreviewAllHandler()
	{
		QSettings inproc("HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler + "\\InProcServer32", QSettings::NativeFormat);
		inproc.remove("");

		QSettings clsidRoot("HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\" + CLSID_PreviewAllHandler, QSettings::NativeFormat);
		clsidRoot.remove("");

		QSettings previewHandlers("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers", QSettings::NativeFormat);
		previewHandlers.remove(CLSID_PreviewAllHandler);
	}

	void registerExtention(const QString& suffix)
	{
		QSettings shellExKey("HKEY_LOCAL_MACHINE\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
		shellExKey.setValue(".", CLSID_PreviewAllHandler);
	}

	void unregisterExtention(const QString& suffix)
	{
		QSettings shellExKey("HKEY_LOCAL_MACHINE\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
		shellExKey.remove("");
	}

	bool isRegisteredExtention(const QString& suffix)
	{
		QSettings shellExKey("HKEY_LOCAL_MACHINE\\Software\\Classes\\" + suffix + "\\ShellEx\\" + CLSID_PreviewHandlerCategory, QSettings::NativeFormat);
		return shellExKey.value(".").toString() == CLSID_PreviewAllHandler;
	}

}


PreviewOptionPanel::PreviewOptionPanel(QSystemTrayIcon* trayIcon /*= nullptr*/, QWidget* parent /*= nullptr*/)
	: QWidget(parent), m_trayIcon(trayIcon)
{
	registerPreviewAllHandler();

	setWindowFlag(Qt::WindowMaximizeButtonHint, false);
	setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
	setFixedSize(320, 320);

	if (m_trayIcon)
		setWindowIcon(m_trayIcon->icon());

	m_mainLayout = new QVBoxLayout(this);
	addSwitchCard(tr("Image Files"), s_imageExtList, &m_imageSwitch);
	addSwitchCard(tr("Archive Files"), s_archiveExtList, &m_archiveSwitch);
	addSwitchCard(tr("Source Code"), s_codeExtList, &m_codeSwitch);
}

PreviewOptionPanel::~PreviewOptionPanel()
{
	unregisterPreviewAllHandler();
}

void PreviewOptionPanel::showEvent(QShowEvent* event)
{
	auto funcGetSwitchCheckState = [](const QStringList& extList)->bool {
		for (const QString& ext : extList)
		{
			if (isRegisteredExtention(ext))
				return true;
		}
		return false;
	};

	if (m_imageSwitch) m_imageSwitch->setChecked(funcGetSwitchCheckState(s_imageExtList));
	if (m_archiveSwitch) m_archiveSwitch->setChecked(funcGetSwitchCheckState(s_archiveExtList));
	if (m_codeSwitch) m_codeSwitch->setChecked(funcGetSwitchCheckState(s_codeExtList));

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
				registerExtention(ext);
			else
				unregisterExtention(ext);
		}
	});

	cardLayout->addWidget(titleLabel);
	cardLayout->addWidget(*switchControl);
	
	if (m_mainLayout)
		m_mainLayout->addWidget(card);
}
