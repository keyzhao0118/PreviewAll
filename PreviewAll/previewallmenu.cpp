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
	m_actImagePreview = addAction(tr("Preview Image"));
	m_actArchivePreview = addAction(tr("Preview Archive"));
	m_actCodePreview = addAction(tr("Preview Code"));
	m_actPdfPreview = addAction(tr("Preview PDF"));

	m_actImagePreview->setCheckable(true);
	m_actArchivePreview->setCheckable(true);
	m_actCodePreview->setCheckable(true);
	m_actPdfPreview->setCheckable(true);

	addSeparator();
	m_actHelp = addAction(tr("Help"));
	addSeparator();
	m_actExit = addAction(tr("Exit"));
}

void PreviewAllMenu::initConnect()
{
	connect(m_actExit, &QAction::triggered, qApp, &QCoreApplication::quit);
	connect(m_actHelp, &QAction::triggered, this, &PreviewAllMenu::showHelpPage);
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

	connect(m_actPdfPreview, &QAction::toggled, this, [](bool checked) {
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::pdfExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::pdfExtList);
		QSettings settings;
		settings.setValue("switchState/pdf", checked);
	});
}

void PreviewAllMenu::initCheckState()
{
	QSettings settings;
	bool imageState = settings.value("switchState/image", false).toBool();
	bool archiveState = settings.value("switchState/archive", false).toBool();
	bool codeState = settings.value("switchState/code", false).toBool();
	bool pdfState = settings.value("switchState/pdf", false).toBool();

	m_actImagePreview->setChecked(imageState);
	m_actArchivePreview->setChecked(archiveState);
	m_actCodePreview->setChecked(codeState);
	m_actPdfPreview->setChecked(pdfState);
}

void PreviewAllMenu::showHelpPage()
{

}




