#include "previewallmenu.h"
#include "previewallregister.h"
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QSettings>

namespace
{
	void updateCheckIcon(QAction* action, bool checked)
	{
		action->setIcon(checked ? QIcon(":/svg/check.svg") : QIcon());
	}
}

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
	m_actMarkdownPreview = addAction(tr("Preview Markdown"));

	m_actImagePreview->setCheckable(true);
	m_actArchivePreview->setCheckable(true);
	m_actMarkdownPreview->setCheckable(true);
	m_actImagePreview->setIconVisibleInMenu(true);
	m_actArchivePreview->setIconVisibleInMenu(true);
	m_actMarkdownPreview->setIconVisibleInMenu(true);

	addSeparator();
	m_actHelp = addAction(tr("Help"));
	addSeparator();
	m_actExit = addAction(tr("Exit"));
}

void PreviewAllMenu::initConnect()
{
	connect(m_actExit, &QAction::triggered, qApp, &QCoreApplication::quit);
	connect(m_actHelp, &QAction::triggered, this, &PreviewAllMenu::showHelpPage);
	connect(m_actImagePreview, &QAction::toggled, this, [this](bool checked) {
		updateCheckIcon(m_actImagePreview, checked);
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::imageExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::imageExtList);
		QSettings settings;
		settings.setValue("switchState/image", checked);
	});

	connect(m_actArchivePreview, &QAction::toggled, this, [this](bool checked) {
		updateCheckIcon(m_actArchivePreview, checked);
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::archiveExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::archiveExtList);
		QSettings settings;
		settings.setValue("switchState/archive", checked);
	});
	connect(m_actMarkdownPreview, &QAction::toggled, this, [this](bool checked) {
		updateCheckIcon(m_actMarkdownPreview, checked);
		if (checked)
			PreviewAllRegister::registerExtentions(PreviewAllRegister::markdownExtList);
		else
			PreviewAllRegister::unregisterExtentions(PreviewAllRegister::markdownExtList);
		QSettings settings;
		settings.setValue("switchState/markdown", checked);
	});

}

void PreviewAllMenu::initCheckState()
{
	QSettings settings;
	bool imageState = settings.value("switchState/image", false).toBool();
	bool archiveState = settings.value("switchState/archive", false).toBool();
	bool markdownState = settings.value("switchState/markdown", false).toBool();

	m_actImagePreview->setChecked(imageState);
	m_actArchivePreview->setChecked(archiveState);
	m_actMarkdownPreview->setChecked(markdownState);
}

void PreviewAllMenu::showHelpPage()
{

}




