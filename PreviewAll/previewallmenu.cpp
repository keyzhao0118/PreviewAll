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
	m_actImagePreview = addAction(tr("Image Preview"));
	m_actArchivePreview = addAction(tr("Archive Preview"));
	m_actCodePreview = addAction(tr("Code Preview"));
	
	m_actImagePreview->setCheckable(true);
	m_actArchivePreview->setCheckable(true);
	m_actCodePreview->setCheckable(true);

	addSeparator();
	m_actExit = addAction(tr("Exit"));
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




