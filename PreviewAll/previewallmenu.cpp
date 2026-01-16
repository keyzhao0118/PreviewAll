#include "previewallmenu.h"
#include "previewallregister.h"
#include <QApplication>

PreviewAllMenu::PreviewAllMenu(QWidget* parent /*= nullptr*/)
	: QMenu(parent)
{
	initUi();
	initConnect();
	initCheckState();
}

PreviewAllMenu::~PreviewAllMenu()
{
}

void PreviewAllMenu::initUi()
{
	setWindowFlag(Qt::FramelessWindowHint, true);
	setWindowFlag(Qt::NoDropShadowWindowHint, true);
	setAttribute(Qt::WA_TranslucentBackground, true);

	m_actImagePreview = addAction(tr("Preview Image"));
	m_actArchivePreview = addAction(tr("Preview Archive"));
	m_actCodePreview = addAction(tr("Code Preview"));
	
	m_actImagePreview->setCheckable(true);
	m_actArchivePreview->setCheckable(true);
	m_actCodePreview->setCheckable(true);

	addSeparator();
	m_actExit = addAction(tr("Exit"));

// 浅色现代风 QSS
	setStyleSheet(R"(
QMenu {
	background-color: #FFFFFF;
	color: #1F2328;
	border: 1px solid #D7D7D7;
	border-radius: 10px;
	padding: 6px;
}

/* 菜单项 */
QMenu::item {
	padding: 8px 34px 8px 34px;   /* 左侧给 indicator 预留空间 */
	margin: 2px 4px;
	border-radius: 8px;
	background: transparent;
}

/* hover / selected */
QMenu::item:selected {
	background-color: #F0F0F0;
}

/* disabled */
QMenu::item:disabled {
	color: #8C959F;
}

/* 分隔线 */
QMenu::separator {
	height: 1px;
	background: #D7D7D7;
	margin: 4px 8px;
}

/* 子菜单箭头（你当前没子菜单；有的话可在这里换图标） */
QMenu::right-arrow {
	image: none;
	width: 0px;
	height: 0px;
}

/* checkable indicator 区域 */
QMenu::indicator {
	width: 16px;
	height: 16px;
	left: 10px;
}

QMenu::indicator:unchecked {
	image: none;
}

QMenu::indicator:checked {
	image: url(:/icons/check.png);
}
)");
}

void PreviewAllMenu::initConnect()
{
	connect(m_actExit, &QAction::triggered, qApp, &QCoreApplication::quit);

	connect(m_actImagePreview, &QAction::toggled, this, [](bool checked) {
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::imageExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::imageExtList);
		QSettings settings;
		settings.setValue("switchState/image", checked);
	});

	connect(m_actArchivePreview, &QAction::toggled, this, [](bool checked) {
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::archiveExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::archiveExtList);
		QSettings settings;
		settings.setValue("switchState/archive", checked);
	});

	connect(m_actCodePreview, &QAction::toggled, this, [](bool checked) {
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::codeExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::codeExtList);
		QSettings settings;
		settings.setValue("switchState/code", checked);
	});
}

void PreviewAllMenu::initCheckState()
{
	QSettings settings;
	bool imageState = settings.value("switchState/image", false).toBool();
	bool archiveState = settings.value("switchState/archive", false).toBool();
	bool codeState = settings.value("switchState/code", false).toBool();

	m_actImagePreview->setChecked(imageState);
	m_actArchivePreview->setChecked(archiveState);
	m_actCodePreview->setChecked(codeState);
}




