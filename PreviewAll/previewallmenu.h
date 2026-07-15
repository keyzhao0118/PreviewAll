#pragma once

#include <QMenu>

class QAction;

class PreviewAllMenu : public QMenu
{
	Q_OBJECT

public:
	explicit PreviewAllMenu(QWidget* parent = nullptr);
	~PreviewAllMenu();

private:
	void initUi();
	void initConnect();
	void initCheckState();

private slots:
	void showHelpPage();

private:
	QAction* m_actImagePreview = nullptr;
	QAction* m_actArchivePreview = nullptr;

	QAction* m_actHelp = nullptr;

	QAction* m_actReboot = nullptr;
	QAction* m_actExit = nullptr;
};